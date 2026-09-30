#include "Environment.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ReadyEvent.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/data/src/Scenario.h"
#include <cassert>
#include <limits>

using namespace BPMNOS::Execution;

Environment::Environment()
  : lastReadyCheckTime(std::numeric_limits<BPMNOS::number>::lowest())
  , lastCompletionCheckTime(std::numeric_limits<BPMNOS::number>::lowest())
{
}

void Environment::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Observable::Type::Event, Observable::Type::Token, Observable::Type::SystemState);
  EventDispatcher::connect(mediator);
}

void Environment::notice(const Observable* observable) {
  if (observable->getObservableType() == Observable::Type::SystemState) {
    // a freshly installed state (e.g. on resume): reset and rebuild from the tokens it lists as awaiting a
    // ready or completion event
    auto systemState = static_cast<const SystemState*>(observable);
    lastReadyCheckTime = std::numeric_limits<BPMNOS::number>::lowest();
    tokensAwaitingReadyEvent.clear();
    pendingReadyEvents.clear();
    for (auto& [token_ptr] : systemState->tokensAwaitingReadyEvent) {
      tokensAwaitingReadyEvent.emplace_back(token_ptr);
    }
    lastCompletionCheckTime = std::numeric_limits<BPMNOS::number>::lowest();
    tokensAwaitingCompletionEvent.clear();
    pendingCompletionEvents.clear();
    for (auto element : systemState->tokensAwaitingCompletionEvent) {
      tokensAwaitingCompletionEvent.emplace_back(std::get<1>(element));
    }
    return;
  }

  // Handle ClockTick events - announce the time the clock is advancing to
  if (observable->getObservableType() == Observable::Type::Event) {
    auto event = static_cast<const Event*>(observable);
    if (auto clockTickEvent = event->is<ClockTickEvent>()) {
      clockTickEvent->systemState->scenario->noticeClockTick(clockTickEvent->time);
    }
    return;
  }

  // Handle Token state changes
  if (observable->getObservableType() == Observable::Type::Token) {
    auto token = static_cast<const Token*>(observable);
    auto scenario = token->owner->systemState->scenario;

    if (!token->node) {
      return;
    }

    // Handle ARRIVED/CREATED at Activity - notify scenario of arrival and await the ready event.
    // Multi-instance instance tokens also pass through CREATED, but the activity's arrival
    // status is computed once for the main token and inherited by the instances; they must not
    // re-trigger the scenario (which would write orphaned arrival statuses and, for stochastic
    // scenarios, consume the activity's random stream), and they receive no ready event of their
    // own: the single ready event fires on the main token, and each instance is materialised from
    // it with its data already known. Instance tokens are the keys of tokenAtMultiInstanceActivity;
    // the main token is only ever a value, so it still passes this guard.
    if (
      token->node->represents<BPMN::Activity>() &&
      (token->state == Token::State::ARRIVED || token->state == Token::State::CREATED) &&
      !token->owner->systemState->tokenAtMultiInstanceActivity.contains(const_cast<Token*>(token))
    ) {
      auto systemState = token->owner->systemState;
      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      // the arrival is announced first, since the ready status is determined from it
      scenario->noticeReadyPending(instanceId, token->node, token->status, *token->data, token->globals);

      auto token_ptr = const_cast<Token*>(token)->weak_from_this();
      // If we've already checked at this time, check new token immediately
      if (lastReadyCheckTime == systemState->getTime()) {
        if (auto event = getReadyEvent(token, systemState)) {
          pendingReadyEvents.emplace_back(token_ptr, event);
        }
        else {
          tokensAwaitingReadyEvent.emplace_back(token_ptr);
        }
      }
      else {
        tokensAwaitingReadyEvent.emplace_back(token_ptr);
      }
    }

    // Handle READY at Activity - the arrival status has been used and is no longer owed. The guard
    // mirrors the one above, so that only an arrival that was announced is discarded.
    if (
      token->node->represents<BPMN::Activity>() &&
      token->state == Token::State::READY &&
      !token->owner->systemState->tokenAtMultiInstanceActivity.contains(const_cast<Token*>(token))
    ) {
      scenario->noticeReady(token->getInstanceId(), token->node);
    }

    // Handle BUSY at Task - notify scenario of running task and await the completion event
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::BUSY
    ) {
      // Exclude SendTask, ReceiveTask, DecisionTask
      if (
        token->node->represents<BPMN::SendTask>() ||
        token->node->represents<BPMNOS::Model::DecisionTask>() ||
        token->node->represents<BPMN::ReceiveTask>()
      ) {
        // these tasks complete through other events
        return;
      }

      auto systemState = token->owner->systemState;
      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      // the running task is announced first, since the completion status is determined from it
      scenario->noticeCompletionPending(instanceId, token->node, token->status, *token->data, token->globals);

      auto token_ptr = const_cast<Token*>(token)->weak_from_this();
      // If we've already checked at this time, check new token immediately
      if (lastCompletionCheckTime == systemState->getTime()) {
        if (auto event = getCompletionEvent(token, systemState)) {
          pendingCompletionEvents.emplace_back(token_ptr, event);
        }
        else {
          tokensAwaitingCompletionEvent.emplace_back(token_ptr);
        }
      }
      else {
        tokensAwaitingCompletionEvent.emplace_back(token_ptr);
      }
    }

    // Handle COMPLETED at Task - the completion status has been used and is no longer owed. The
    // announcement came from either of two places, so the guard is not the one used for BUSY above:
    // the environment announces every task that is not a send, receive or decision task, while
    // Token::advanceToCompleted announces every send task and every receive or decision task that
    // carries extension elements. A receive or decision task without them is announced by neither.
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::COMPLETED
    ) {
      bool announced =
        token->node->represents<BPMN::SendTask>() ||
        (
          ( token->node->represents<BPMN::ReceiveTask>() || token->node->represents<BPMNOS::Model::DecisionTask>() )
          ? (bool)token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>()
          : true
        );
      if ( announced ) {
        scenario->noticeCompletion(token->getInstanceId(), token->node);
      }
    }
  }
}

std::shared_ptr<Event> Environment::getReadyEvent(const Token* token, const SystemState* systemState) {
  auto rootId = token->owner->root->instance.value();
  auto instanceId = token->getInstanceId();
  auto currentTime = systemState->getTime();

  auto status = systemState->scenario->getActivityReadyStatus(rootId, instanceId, token->node, currentTime);
  auto data = systemState->getDataAttributes(token->owner->root, token->node);

  if (status.has_value() && data.has_value()) {
    return std::make_shared<ReadyEvent>(const_cast<Token*>(token), std::move(*status), std::move(*data));
  }
  return nullptr;
}

std::shared_ptr<Event> Environment::dispatchEvent(const SystemState* systemState) {
  if (auto event = dispatchReadyEvent(systemState)) {
    return event;
  }
  return dispatchCompletionEvent(systemState);
}

std::shared_ptr<Event> Environment::dispatchReadyEvent(const SystemState* systemState) {
  // Dispatch from pending first
  for (auto& [token_ptr, event] : pendingReadyEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingReadyEvents.remove(token.get());
      return result;
    }
  }

  auto currentTime = systemState->getTime();
  if (currentTime == lastReadyCheckTime) {
    return nullptr;
  }
  lastReadyCheckTime = currentTime;

  // Check all awaiting tokens
  std::vector<Token*> readyTokens;
  for (auto& [token_ptr] : tokensAwaitingReadyEvent) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getReadyEvent(token.get(), systemState)) {
        readyTokens.push_back(token.get());
        pendingReadyEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : readyTokens) {
    tokensAwaitingReadyEvent.remove(token);
  }

  // Dispatch first pending
  for (auto& [token_ptr, event] : pendingReadyEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingReadyEvents.remove(token.get());
      return result;
    }
  }

  return nullptr;
}

std::shared_ptr<Event> Environment::getCompletionEvent(const Token* token, const SystemState* systemState) {
  auto instanceId = token->getInstanceId();
  auto currentTime = systemState->getTime();

  auto status = systemState->scenario->getTaskCompletionStatus(instanceId, token->node, currentTime);

  if (status.has_value()) {
    return std::make_shared<CompletionEvent>(const_cast<Token*>(token), std::move(*status));
  }
  return nullptr;
}

std::shared_ptr<Event> Environment::dispatchCompletionEvent(const SystemState* systemState) {
  // Dispatch from pending first
  for (auto& [token_ptr, event] : pendingCompletionEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingCompletionEvents.remove(token.get());
      return result;
    }
  }

  auto currentTime = systemState->getTime();
  if (currentTime == lastCompletionCheckTime) {
    return nullptr;
  }
  lastCompletionCheckTime = currentTime;

  // Check all awaiting tokens
  std::vector<Token*> completedTokens;
  for (auto& [token_ptr] : tokensAwaitingCompletionEvent) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getCompletionEvent(token.get(), systemState)) {
        completedTokens.push_back(token.get());
        pendingCompletionEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : completedTokens) {
    tokensAwaitingCompletionEvent.remove(token);
  }

  // Dispatch first pending
  for (auto& [token_ptr, event] : pendingCompletionEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingCompletionEvents.remove(token.get());
      return result;
    }
  }

  return nullptr;
}
