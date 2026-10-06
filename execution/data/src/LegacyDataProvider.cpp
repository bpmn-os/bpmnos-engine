#include "LegacyDataProvider.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/events/InstantiationEvent.h"
#include "execution/engine/src/events/SignalBroadcastEvent.h"
#include "execution/engine/src/events/ReadyEvent.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "model/bpmnos/src/DecisionTask.h"
#include <cassert>
#include <stdexcept>
#include <limits>

using namespace BPMNOS::Execution;

LegacyDataProvider::Scenario::Scenario(std::shared_ptr<const LegacyDataProvider> dataProvider, const BPMNOS::Model::Scenario* scenario)
  : Execution::Scenario(std::move(dataProvider))
  , scenario(scenario)
  , previousTime(std::numeric_limits<BPMNOS::number>::lowest())
  , instantiationsDue(false)
  , signalsDue(false)
  , lastReadyCheckTime(std::numeric_limits<BPMNOS::number>::lowest())
  , lastCompletionCheckTime(std::numeric_limits<BPMNOS::number>::lowest())
  , clockTickProcessed(false)
{
}

LegacyDataProvider::LegacyDataProvider(const BPMNOS::Model::Model* model, unsigned int clockTickDuration, BPMNOS::number endTime)
  : DataProvider(model, clockTickDuration)
  , endTime(endTime)
{
}

std::unique_ptr<LegacyDataProvider::Scenario> LegacyDataProvider::wrap(const BPMNOS::Model::Scenario* scenario, BPMNOS::number endTime) {
  return std::make_shared<const LegacyDataProvider>(scenario->getModel(), 0, endTime)->createScenario(scenario);
}

void LegacyDataProvider::advance(const SystemState* systemState, Execution::Scenario& scenario, EventQueue& queue) const {
  if ( systemState->getTime() >= endTime ) {
    // nothing is left to do at the end time
    queue.push_back(std::make_shared<TerminationEvent>());
    return;
  }
  DataProvider::advance(systemState, scenario, queue);
}

std::unique_ptr<LegacyDataProvider::Scenario> LegacyDataProvider::createScenario(const BPMNOS::Model::Scenario* scenario) const {
  if ( scenario->getModel() != getModel() ) {
    throw std::invalid_argument("LegacyDataProvider: the scenario is not one of the model of the data provider");
  }
  return std::make_unique<Scenario>(std::static_pointer_cast<const LegacyDataProvider>(shared_from_this()), scenario);
}

std::unique_ptr<BPMNOS::Execution::Scenario> LegacyDataProvider::createScenario([[maybe_unused]] unsigned int realisation) const {
  throw std::logic_error("LegacyDataProvider: a scenario is created by wrapping a Model::Scenario");
}

BPMNOS::Values LegacyDataProvider::getGlobals(const Execution::Scenario& executionScenario) const {
  return static_cast<const Scenario&>(executionScenario).scenario->globals;
}

BPMNOS::number LegacyDataProvider::getEarliestInstantiationTime(const Execution::Scenario& executionScenario) const {
  return static_cast<const Scenario&>(executionScenario).scenario->getEarliestInstantiationTime();
}

bool LegacyDataProvider::isAlive(const SystemState* systemState, const Scenario& scenario) {
  if ( !scenario.scenario->isCompleted(systemState->getTime()) ) {
    return true;
  }
  return !systemState->instances.empty();
}

void LegacyDataProvider::dispatchEvent(const SystemState* systemState, Execution::Scenario& executionScenario, EventQueue& queue) const {
  auto& record = static_cast<Scenario&>(executionScenario);
  if ( record.clockTickProcessed ) {
    record.clockTickProcessed = false;
    if ( !isAlive(systemState, record) ) {
      // the run ends once a clock tick leaves the scenario completed and no instance
      queue.push_back(std::make_shared<TerminationEvent>());
      return;
    }
  }
  if (auto event = determineEvent(systemState, record)) {
    queue.push_back(event);
  }
}

void LegacyDataProvider::notice(const Observable* observable, Execution::Scenario& executionScenario, [[maybe_unused]] EventQueue& queue) const {
  auto& record = static_cast<Scenario&>(executionScenario);
  if (observable->getObservableType() == Observable::Type::SystemState) {
    // a freshly installed state (e.g. on resume): reset and rebuild from the tokens it lists as awaiting a
    // ready or completion event
    auto systemState = static_cast<const SystemState*>(observable);
    // the state holds every instance known up to its time, those becoming known later are determined at
    // the next clock tick
    record.pendingInstantiationEvents.clear();
    record.instantiationsDue = false;
    record.pendingSignalBroadcastEvents.clear();
    record.signalsDue = false;
    record.lastReadyCheckTime = std::numeric_limits<BPMNOS::number>::lowest();
    record.processTokensAwaitingReadyEvent.clear();
    record.tokensAwaitingReadyEvent.clear();
    record.pendingReadyEvents.clear();
    for (auto& [token_ptr] : systemState->tokensAwaitingReadyEvent) {
      if (auto token = token_ptr.lock(); token && !token->node) {
        record.processTokensAwaitingReadyEvent.emplace_back(token_ptr);
      }
      else {
        record.tokensAwaitingReadyEvent.emplace_back(token_ptr);
      }
    }
    record.lastCompletionCheckTime = std::numeric_limits<BPMNOS::number>::lowest();
    record.tokensAwaitingCompletionEvent.clear();
    record.pendingCompletionEvents.clear();
    for (auto element : systemState->tokensAwaitingCompletionEvent) {
      record.tokensAwaitingCompletionEvent.emplace_back(std::get<1>(element));
    }
    return;
  }

  // Handle ClockTick events - announce the time the clock is advancing to
  if (observable->getObservableType() == Observable::Type::Event) {
    auto event = static_cast<const Event*>(observable);
    if (auto clockTickEvent = event->is<ClockTickEvent>()) {
      record.scenario->noticeClockTick(clockTickEvent->time);
      // the tick is announced before time advances, so the state still holds the previous time
      record.previousTime = clockTickEvent->systemState->getTime();
      record.instantiationsDue = true;
      record.signalsDue = true;
      // the clock tick is announced before it is processed, so it has been processed when events are next dispatched
      record.clockTickProcessed = true;
    }
    return;
  }

  // Handle Token state changes
  if (observable->getObservableType() == Observable::Type::Token) {
    auto token = static_cast<const Token*>(observable);
    auto scenario = record.scenario;

    if (!token->node) {
      // Handle CREATED at Process - await the ready event starting the instance. Tokens at processes are
      // checked before those at activities, so an instance starts before any activity becomes ready at
      // the same instant.
      if (token->state == Token::State::CREATED) {
        auto systemState = token->owner->systemState;
        auto token_ptr = const_cast<Token*>(token)->weak_from_this();
        // If we've already checked at this time, check new token immediately
        if (record.lastReadyCheckTime == systemState->getTime()) {
          if (auto event = getReadyEvent(token, systemState, record)) {
            record.pendingReadyEvents.emplace_back(token_ptr, event);
          }
          else {
            record.processTokensAwaitingReadyEvent.emplace_back(token_ptr);
          }
        }
        else {
          record.processTokensAwaitingReadyEvent.emplace_back(token_ptr);
        }
      }
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
      if (record.lastReadyCheckTime == systemState->getTime()) {
        if (auto event = getReadyEvent(token, systemState, record)) {
          record.pendingReadyEvents.emplace_back(token_ptr, event);
        }
        else {
          record.tokensAwaitingReadyEvent.emplace_back(token_ptr);
        }
      }
      else {
        record.tokensAwaitingReadyEvent.emplace_back(token_ptr);
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
      if (record.lastCompletionCheckTime == systemState->getTime()) {
        if (auto event = getCompletionEvent(token, systemState, record)) {
          record.pendingCompletionEvents.emplace_back(token_ptr, event);
        }
        else {
          record.tokensAwaitingCompletionEvent.emplace_back(token_ptr);
        }
      }
      else {
        record.tokensAwaitingCompletionEvent.emplace_back(token_ptr);
      }
    }

    // Handle COMPLETED at Task - the completion status has been used and is no longer owed. The guard
    // mirrors the one used for BUSY above, so that only a completion that was announced is discarded: a
    // send, receive or decision task completes without the scenario.
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::COMPLETED &&
      !token->node->represents<BPMN::SendTask>() &&
      !token->node->represents<BPMN::ReceiveTask>() &&
      !token->node->represents<BPMNOS::Model::DecisionTask>()
    ) {
      scenario->noticeCompletion(token->getInstanceId(), token->node);
    }
  }
}

std::shared_ptr<Event> LegacyDataProvider::getReadyEvent(const Token* token, const SystemState* systemState, Scenario& record) const {
  if (!token->node) {
    // the token at a process is ready once the scenario discloses the status and data of the process
    auto instanceId = token->owner->root->instance.value();
    auto status = record.scenario->getProcessReadyStatus(instanceId, systemState->getTime());
    auto data = record.scenario->getData(token->owner->root->instance.value(), token->owner->process, systemState->getTime());
    if (status.has_value() && data.has_value()) {
      return std::make_shared<ReadyEvent>(const_cast<Token*>(token), std::move(*status), std::move(*data));
    }
    return nullptr;
  }

  auto rootId = token->owner->root->instance.value();
  auto instanceId = token->getInstanceId();
  auto currentTime = systemState->getTime();

  auto status = record.scenario->getActivityReadyStatus(rootId, instanceId, token->node, currentTime);
  auto data = record.scenario->getData(token->owner->root->instance.value(), token->node, systemState->getTime());

  if (status.has_value() && data.has_value()) {
    return std::make_shared<ReadyEvent>(const_cast<Token*>(token), std::move(*status), std::move(*data));
  }
  return nullptr;
}

std::shared_ptr<Event> LegacyDataProvider::determineEvent(const SystemState* systemState, Scenario& record) const {
  // instances are created before any ready event is determined, so that their tokens are checked too
  if (auto event = dispatchInstantiationEvent(systemState, record)) {
    return event;
  }
  if (auto event = dispatchReadyEvent(systemState, record)) {
    return event;
  }
  if (auto event = dispatchCompletionEvent(systemState, record)) {
    return event;
  }
  // signals are raised once everything else the environment supplies at this instant has been processed
  return dispatchSignalBroadcastEvent(systemState, record);
}

std::shared_ptr<Event> LegacyDataProvider::dispatchInstantiationEvent(const SystemState* systemState, Scenario& record) const {
  if (record.instantiationsDue) {
    record.instantiationsDue = false;
    for (auto& [process, status, data] : record.scenario->getKnownInstantiations(record.previousTime, systemState->getTime())) {
      record.pendingInstantiationEvents.push_back(std::make_shared<InstantiationEvent>(process, std::move(status), std::move(data)));
    }
  }

  if (record.pendingInstantiationEvents.empty()) {
    return nullptr;
  }
  auto event = record.pendingInstantiationEvents.front();
  record.pendingInstantiationEvents.pop_front();
  return event;
}

std::shared_ptr<Event> LegacyDataProvider::dispatchSignalBroadcastEvent(const SystemState* systemState, Scenario& record) const {
  if (record.signalsDue) {
    record.signalsDue = false;
    for (auto& [name, content] : record.scenario->getSignals(record.previousTime, systemState->getTime())) {
      record.pendingSignalBroadcastEvents.push_back(std::make_shared<SignalBroadcastEvent>(Signal(name, std::move(content))));
    }
  }

  if (record.pendingSignalBroadcastEvents.empty()) {
    return nullptr;
  }
  auto event = record.pendingSignalBroadcastEvents.front();
  record.pendingSignalBroadcastEvents.pop_front();
  return event;
}

std::shared_ptr<Event> LegacyDataProvider::dispatchReadyEvent(const SystemState* systemState, Scenario& record) const {
  // Dispatch from pending first
  for (auto& [token_ptr, event] : record.pendingReadyEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      record.pendingReadyEvents.remove(token.get());
      return result;
    }
  }

  auto currentTime = systemState->getTime();
  if (currentTime == record.lastReadyCheckTime) {
    return nullptr;
  }
  record.lastReadyCheckTime = currentTime;

  // Check all awaiting tokens, those at processes first
  std::vector<Token*> readyTokens;
  for (auto& [token_ptr] : record.processTokensAwaitingReadyEvent) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getReadyEvent(token.get(), systemState, record)) {
        readyTokens.push_back(token.get());
        record.pendingReadyEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : readyTokens) {
    record.processTokensAwaitingReadyEvent.remove(token);
  }
  readyTokens.clear();
  for (auto& [token_ptr] : record.tokensAwaitingReadyEvent) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getReadyEvent(token.get(), systemState, record)) {
        readyTokens.push_back(token.get());
        record.pendingReadyEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : readyTokens) {
    record.tokensAwaitingReadyEvent.remove(token);
  }

  // Dispatch first pending
  for (auto& [token_ptr, event] : record.pendingReadyEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      record.pendingReadyEvents.remove(token.get());
      return result;
    }
  }

  return nullptr;
}

std::shared_ptr<Event> LegacyDataProvider::getCompletionEvent(const Token* token, const SystemState* systemState, Scenario& record) const {
  auto instanceId = token->getInstanceId();
  auto currentTime = systemState->getTime();

  auto status = record.scenario->getTaskCompletionStatus(instanceId, token->node, currentTime);

  if (status.has_value()) {
    return std::make_shared<CompletionEvent>(const_cast<Token*>(token), std::move(*status));
  }
  return nullptr;
}

std::shared_ptr<Event> LegacyDataProvider::dispatchCompletionEvent(const SystemState* systemState, Scenario& record) const {
  // Dispatch from pending first
  for (auto& [token_ptr, event] : record.pendingCompletionEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      record.pendingCompletionEvents.remove(token.get());
      return result;
    }
  }

  auto currentTime = systemState->getTime();
  if (currentTime == record.lastCompletionCheckTime) {
    return nullptr;
  }
  record.lastCompletionCheckTime = currentTime;

  // Check all awaiting tokens
  std::vector<Token*> completedTokens;
  for (auto& [token_ptr] : record.tokensAwaitingCompletionEvent) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getCompletionEvent(token.get(), systemState, record)) {
        completedTokens.push_back(token.get());
        record.pendingCompletionEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : completedTokens) {
    record.tokensAwaitingCompletionEvent.remove(token);
  }

  // Dispatch first pending
  for (auto& [token_ptr, event] : record.pendingCompletionEvents) {
    if (auto token = token_ptr.lock()) {
      auto result = event;  // Copy before remove destroys the tuple
      record.pendingCompletionEvents.remove(token.get());
      return result;
    }
  }

  return nullptr;
}
