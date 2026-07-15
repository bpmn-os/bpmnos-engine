#include "SystemState.h"
#include "Engine.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
  , contributionsToObjective(0)
  , globals(scenario->globals)
{
}

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, const SystemState* other)
  : engine(engine)
  , scenario(scenario)
  , currentTime(other->currentTime)
  , contributionsToObjective(other->contributionsToObjective)
  , globals(other->globals)
  , instantiationCounter(other->instantiationCounter)
{
  // Copy root state machine instances
  for (const auto& otherInstance : other->instances) {
    instances.push_back(std::make_shared<StateMachine>(this, nullptr, otherInstance.get()));

    // Populate archive
    auto key = (long unsigned int)otherInstance->instance.value();
    if (other->archive.contains(key) && other->archive.at(key).lock().get() == otherInstance.get()) {
      archive[key] = instances.back();
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

bool SystemState::isAlive() const {
  if ( !scenario->isCompleted(getTime()) ) {
    return true;
  }
  return !instances.empty();
};

BPMNOS::number SystemState::getWeightedObjective() const {
  BPMNOS::number result = 0.0;
  for ( auto& [attribute, value] : contributionsToObjective ) {
    result += value * attribute->weight;
  }
  
  for ( auto& attribute : scenario->getModel()->attributes ) {
    assert( attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL );
    auto value = globals[attribute->index];
    if ( value.has_value() ) {
      result += value.value() * attribute->weight;
    }
  }
  
  return result;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > SystemState::getInstantiations() const {
  return scenario->getCurrentInstantiations(currentTime);
}

std::optional<BPMNOS::Values> SystemState::getStatusAttributes(const StateMachine* root, const BPMN::Node* node) const {
  return scenario->getStatus(root->instance.value(), node, currentTime);
}

std::optional<BPMNOS::Values> SystemState::getDataAttributes(const StateMachine* root, const BPMN::Node* node) const {
  return scenario->getData(root->instance.value(), node, currentTime);
}

void SystemState::increaseTimeTo(BPMNOS::number time) {
  assert(time > currentTime);
  currentTime = time;
}

