#include "ReadyHandler.h"
#include "Mediator.h"
#include "Token.h"
#include "SystemState.h"
#include "events/ReadyEvent.h"
#include <cassert>
#include <limits>

using namespace BPMNOS::Execution;

ReadyHandler::ReadyHandler()
  : lastCheckedTime(std::numeric_limits<BPMNOS::number>::lowest())
{
}

ReadyHandler::~ReadyHandler() {
}

void ReadyHandler::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Observable::Type::Token);
  EventDispatcher::connect(mediator);
}

void ReadyHandler::notice(const Observable* observable) {
  if (observable->getObservableType() != Observable::Type::Token) {
    return;
  }

  auto token = static_cast<const Token*>(observable);

  // Filter: Activity nodes in ARRIVED or CREATED state
  if (!token->node ||
      !token->node->represents<BPMN::Activity>() ||
      (token->state != Token::State::ARRIVED && token->state != Token::State::CREATED)) {
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

std::shared_ptr<Event> ReadyHandler::getEvent(const Token* token, const SystemState* systemState) {
  auto instanceId = token->owner->root->instance.value();
  auto currentTime = systemState->getTime();

  auto status = systemState->scenario->getActivityReadyStatus(instanceId, token->node, currentTime);
  auto data = systemState->getDataAttributes(token->owner->root, token->node);

  if (status.has_value() && data.has_value()) {
    return std::make_shared<ReadyEvent>(const_cast<Token*>(token), std::move(*status), std::move(*data));
  }
  return nullptr;
}

std::shared_ptr<Event> ReadyHandler::dispatchEvent(const SystemState* systemState) {
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
  std::vector<Token*> readyTokens;
  for (auto& [token_ptr] : awaitingTokens) {
    if (auto token = token_ptr.lock()) {
      if (auto event = getEvent(token.get(), systemState)) {
        readyTokens.push_back(token.get());
        pendingEvents.emplace_back(token, event);
      }
    }
  }
  for (auto* token : readyTokens) {
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
