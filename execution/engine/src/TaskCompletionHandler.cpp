#include "TaskCompletionHandler.h"
#include "Mediator.h"
#include "Token.h"
#include "SystemState.h"
#include "events/CompletionEvent.h"
#include "model/bpmnos/src/DecisionTask.h"
#include <limits>

using namespace BPMNOS::Execution;

TaskCompletionHandler::TaskCompletionHandler()
  : lastCheckedTime(std::numeric_limits<BPMNOS::number>::lowest())
{
}

TaskCompletionHandler::~TaskCompletionHandler() {
}

void TaskCompletionHandler::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Observable::Type::Token);
  EventDispatcher::connect(mediator);
}

void TaskCompletionHandler::notice(const Observable* observable) {
  if (observable->getObservableType() != Observable::Type::Token) {
    return;
  }

  auto token = static_cast<const Token*>(observable);

  // Filter: Task nodes in BUSY state, excluding special tasks
  if (!token->node ||
      !token->node->represents<BPMN::Task>() ||
      token->state != Token::State::BUSY) {
    return;
  }

  // Exclude tasks that complete via external events
  if (token->node->represents<BPMN::SendTask>() ||
      token->node->represents<BPMN::ReceiveTask>() ||
      token->node->represents<BPMNOS::Model::DecisionTask>()) {
    return;
  }

  auto systemState = token->owner->systemState;
  auto currentTime = systemState->getTime();
  auto token_ptr = const_cast<Token*>(token)->weak_from_this();

  // If we've already checked at this time, check new token immediately
  if (lastCheckedTime == currentTime) {
    if (auto event = getEvent(token, systemState)) {
      pendingEvents.emplace_back(token_ptr, event);
    }
    else {
      awaitingTokens.emplace_back(token_ptr);
    }
  }
  else {
    awaitingTokens.emplace_back(token_ptr);
  }
}

std::shared_ptr<Event> TaskCompletionHandler::getEvent(const Token* token, const SystemState* systemState) {
  auto instanceId = token->owner->root->instance.value();
  auto currentTime = systemState->getTime();

  auto status = systemState->scenario->getTaskCompletionStatus(instanceId, token->node, currentTime);

  if (status.has_value()) {
    return std::make_shared<CompletionEvent>(const_cast<Token*>(token), std::move(*status));
  }
  return nullptr;
}

std::shared_ptr<Event> TaskCompletionHandler::dispatchEvent(const SystemState* systemState) {
  // Dispatch from pending first
  for (auto& [token_ptr, event] : pendingEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingEvents.remove(token.get());
      return result;
    }
  }

  auto currentTime = systemState->getTime();
  if (currentTime == lastCheckedTime) {
    return nullptr;
  }
  lastCheckedTime = currentTime;

  // Check all awaiting tokens
  std::vector<Token*> completedTokens;
  for (auto& [token_ptr] : awaitingTokens) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getEvent(token.get(), systemState)) {
        completedTokens.push_back(token.get());
        pendingEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : completedTokens) {
    awaitingTokens.remove(token);
  }

  // Dispatch first pending
  for (auto& [token_ptr, event] : pendingEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      pendingEvents.remove(token.get());
      return result;
    }
  }

  return nullptr;
}
