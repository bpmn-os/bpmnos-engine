#include "Deferrals.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/DataUpdate.h"
#include "execution/engine/src/Message.h"
#include "execution/engine/src/MessageDeliveryRequest.h"
#include "execution/data/src/DataProvider.h"
#include "execution/data/src/Scenario.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

using namespace BPMNOS::Execution;

void Deferrals::connect(Notifier* notifier) {
  notifier->addSubscriber(this,
    Observable::Type::EntryRequest,
    Observable::Type::ChoiceRequest,
    Observable::Type::MessageDeliveryRequest,
    Observable::Type::Message,
    Observable::Type::DataUpdate,
    Observable::Type::SystemState
  );
}

void Deferrals::notice(const Observable* observable) {
  switch ( observable->getObservableType() ) {
    case Observable::Type::SystemState:
      // none of the deferred tokens exists in an installed state
      systemState = static_cast<const SystemState*>(observable);
      deferrals.clear();
      break;
    case Observable::Type::EntryRequest:
    case Observable::Type::ChoiceRequest:
    case Observable::Type::MessageDeliveryRequest:
      // a request arriving at a deferred decision point is a new candidate, and the first request of a token taking
      // the address of a destroyed token ends the stale deferral of the latter
      if ( !deferrals.empty() ) {
        deferrals.erase( decisionPoint(static_cast<const DecisionRequest*>(observable)->token) );
      }
      break;
    case Observable::Type::Message:
      message(static_cast<const Message*>(observable));
      break;
    case Observable::Type::DataUpdate:
      dataUpdate(static_cast<const DataUpdate*>(observable));
      break;
    default:
      break;
  }
}

void Deferrals::defer(const Token* token) {
  auto point = decisionPoint(token);
  auto deferral = deferralOf(point);
  if ( !deferral ) {
    throw std::runtime_error("Deferrals: no deferral stated for the decision point at '" + point->node->id + "'");
  }

  std::vector< std::weak_ptr<const DecisionRequest> > requests;
  if ( point->decisionRequest && point->decisionRequest->type != Observable::Type::EntryRequest ) {
    requests.push_back(point->decisionRequest);
  }
  else {
    // the entry requests of the tokens awaiting entry at the performer
    for ( auto& [ token_ptr ] : point->pendingSequentialEntries ) {
      if ( auto waitingToken = token_ptr.lock(); waitingToken && waitingToken->decisionRequest ) {
        requests.push_back(waitingToken->decisionRequest);
      }
    }
  }
  if ( requests.empty() ) {
    throw std::runtime_error("Deferrals: no pending request at the decision point at '" + point->node->id + "'");
  }

  deferrals.insert_or_assign(point, Entry{ std::move(requests), deferral });
}

bool Deferrals::isDeferred(const Token* token) const {
  return !deferrals.empty() && deferrals.contains(decisionPoint(token));
}

bool Deferrals::isAdmissible(const BPMNOS::Model::Deferral* deferral) const {
  assert( systemState );
  return deferral->timeDependent || systemState->isAwaitingEvents() || systemState->scenario->dataProvider->expectsData(*systemState->scenario);
}

const std::unordered_map< const Token*, Deferrals::Entry >& Deferrals::getDeferrals() const {
  return deferrals;
}

void Deferrals::message(const Message* message) {
  if ( deferrals.empty() || message->state != Message::State::CREATED ) {
    return;
  }
  // a message which may be delivered to a deferred catch event is a new candidate, as message deliveries pair them
  auto& recipientCandidates = message->origin->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  std::erase_if(deferrals, [message, &recipientCandidates](const auto& item) {
    auto& [token, entry] = item;
    auto request = std::get<0>(entry).front().lock();
    return
      request &&
      request->type == Observable::Type::MessageDeliveryRequest &&
      std::ranges::contains(recipientCandidates, token->node) &&
      message->matches(static_cast<const MessageDeliveryRequest*>(request.get())->recipientHeader)
    ;
  });
}

void Deferrals::dataUpdate(const DataUpdate* update) {
  if ( deferrals.empty() ) {
    return;
  }

  // a written global attribute concerns the deferrals of every instance, any other written attribute only those of the
  // instance that wrote it
  std::vector<const BPMNOS::Model::Attribute*> globalAttributes;
  std::ranges::copy_if(update->attributes, std::back_inserter(globalAttributes), [](const BPMNOS::Model::Attribute* attribute) { return attribute->isGlobal; });

  std::erase_if(deferrals, [this, update, &globalAttributes](const auto& item) {
    auto& [token, entry] = item;
    if ( !isAlive(entry) ) {
      return true;
    }
    auto deferral = std::get<const BPMNOS::Model::Deferral*>(entry);
    auto& attributes = ( instanceOf(token) == update->instanceId ) ? update->attributes : globalAttributes;
    if ( std::ranges::any_of(attributes, [deferral](auto attribute) { return std::ranges::contains(deferral->observables, attribute); }) ) {
      return true;
    }
    if (
      std::ranges::any_of(attributes, [deferral](auto attribute) { return deferral->condition.inputs.contains(attribute); }) &&
      !deferral->conditionHolds(token->status, *token->data)
    ) {
      return true;
    }
    // the update of the time at a clock tick, which names the instance 0, ends a deferral no reconsideration of which
    // can come any more
    return update->instanceId == BPMNOS::number(0) && !isAdmissible(deferral);
  });
}

bool Deferrals::isAlive(const Entry& entry) {
  return std::ranges::any_of(std::get<0>(entry), [](auto& request_ptr) { return !request_ptr.expired(); });
}

const Token* Deferrals::decisionPoint(const Token* token) {
  // a token awaiting entry at a sequential activity is represented by its performer token, any other token by itself
  if ( BPMNOS::Model::Model::isSequentialActivity(token->node) && token->state == Token::State::READY ) {
    return token->getSequentialPerformerToken();
  }
  return token;
}

const BPMNOS::Model::Deferral* Deferrals::deferralOf(const Token* token) {
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( token->decisionRequest && token->decisionRequest->type == Observable::Type::ChoiceRequest ) {
    return extensionElements->choiceGuidance.has_value() ? extensionElements->choiceGuidance.value()->deferral.get() : nullptr;
  }
  if ( token->decisionRequest && token->decisionRequest->type == Observable::Type::MessageDeliveryRequest ) {
    return extensionElements->messageDeliveryGuidance.has_value() ? extensionElements->messageDeliveryGuidance.value()->deferral.get() : nullptr;
  }
  return extensionElements->performerDeferral.get();
}

BPMNOS::number Deferrals::instanceOf(const Token* token) {
  // a token at a process is owned by the global state machine and owns the state machine of its instance
  return token->owner->root ? token->owner->root->instance.value() : token->owned->instance.value();
}
