#include "SystemState.h"
#include "Engine.h"
#include "execution/data/src/DataProvider.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
  , objective(0)
  , stateMachine(std::make_shared<StateMachine>(this, scenario->dataProvider->getGlobals(*scenario)))
{
  // the values the globals are created with never pass through setValue, so the objective is seeded with
  // them here; no change is notified, the run not having begun and no token being able to observe it
  auto& globals = stateMachine->ownedData;
  for ( auto& attribute : engine->getModel()->attributes ) {
    if ( attribute->weight != 0 && globals[attribute->index].has_value() ) {
      objective += globals[attribute->index].value() * attribute->weight;
    }
  }
}

SystemState::SystemState(const Engine* engine, const Scenario* scenario, const SystemState* other)
  : engine(engine)
  , scenario(scenario)
  , currentTime(other->currentTime)
  , objective(other->objective)
  , instantiationCounter(other->instantiationCounter)
{
  // Copy the global state machine, which copies the token at the process of each instance together with
  // the state machine of the instance it owns
  stateMachine = std::make_shared<StateMachine>(this, nullptr, other->stateMachine.get());

  // Populate archive with the state machines of the instances, the tokens being copied in their order
  for ( size_t i = 0; i < stateMachine->tokens.size(); i++ ) {
    auto otherInstance = other->stateMachine->tokens[i]->owned.get();
    auto key = (long unsigned int)otherInstance->instance.value();
    if (other->archive.contains(key) && other->archive.at(key).lock().get() == otherInstance) {
      archive[key] = stateMachine->tokens[i]->owned;
    }
  }

  // Copy messages sent from MessageThrowEvent, messages sent from SendTask are created when copying the associated Token
  for (const auto& otherMessage : other->messages) {
    if (!otherMessage->waitingToken) {
      messages.push_back(std::make_shared<Message>(otherMessage.get()));
    }
  }

  // Helper to find new message corresponding to original message
  auto findNewMessage = [this](const Message* otherMessage) -> std::shared_ptr<Message> {
    for (const auto& message : messages) {
      if (message->origin != otherMessage->origin) continue;
      if (message->recipient != otherMessage->recipient) continue;
      // Match waitingToken by node (both nullptr or same node)
      if (otherMessage->waitingToken) {
        if (!message->waitingToken || message->waitingToken->node != otherMessage->waitingToken->node) continue;
      }
      else {
        if (message->waitingToken) continue;
      }
      return message;
    }
    assert(false && "Unable to find message!");
    return nullptr;
  };

  // Populate outbox (keyed by origin node)
  for (const auto& [node, otherMessages] : other->outbox) {
    for (const auto& [otherMessageWeak] : otherMessages) {
      if (auto otherMessage = otherMessageWeak.lock()) {
        if (auto message = findNewMessage(otherMessage.get())) {
          outbox[node].emplace_back(message);
        }
      }
    }
  }

  // Populate unsent (keyed by recipient ID)
  for (const auto& [recipientId, otherMessages] : other->unsent) {
    for (const auto& [otherMessageWeak] : otherMessages) {
      if (auto otherMessage = otherMessageWeak.lock()) {
        if (auto message = findNewMessage(otherMessage.get())) {
          unsent[recipientId].emplace_back(message);
        }
      }
    }
  }

  // Populate inbox (keyed by StateMachine*, use archive for remapping)
  for (const auto& [otherStateMachine, otherMessages] : other->inbox) {
    auto instanceId = (long unsigned int)otherStateMachine->instance.value();
    auto it = archive.find(instanceId);
    if (it == archive.end()) continue;
    auto newStateMachine = it->second.lock();
    if (!newStateMachine) continue;

    for (const auto& [otherMessageWeak] : otherMessages) {
      if (auto otherMessage = otherMessageWeak.lock()) {
        if (auto message = findNewMessage(otherMessage.get())) {
          inbox[newStateMachine.get()].emplace_back(message);
        }
      }
    }
  }
}

SystemState::~SystemState() {
//std::cerr << "~SystemState()" << std::endl;
  inbox.clear();
/*
  tokensAwaitingBoundaryEvent.clear();
  tokenAtAssociatedActivity.clear();
  tokensAwaitingStateMachineCompletion.clear();
  tokensAwaitingGatewayActivation.clear();
  tokensAwaitingJobEntryEvent.clear();
*/
}

BPMNOS::number SystemState::getTime() const {
  return currentTime;
}

BPMNOS::number SystemState::getObjective() const {
  return objective;
}

void SystemState::increaseTimeTo(BPMNOS::number time) {
  assert(time > currentTime);
  currentTime = time;
}

