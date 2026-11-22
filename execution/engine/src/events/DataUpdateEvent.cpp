#include "DataUpdateEvent.h"
#include <algorithm>
#include <stdexcept>
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Execution;

DataUpdateEvent::DataUpdateEvent(const SystemState* systemState, Values values, Objects objects)
  : Event(nullptr)
  , systemState(systemState)
  , scope(nullptr)
  , instance(0)
  , values(std::move(values))
  , objects(std::move(objects))
{
  for ( auto& [attribute, _] : this->values ) {
    validate(attribute, false);
  }
  for ( auto& [object, _] : this->objects ) {
    validate(object, true);
  }
}

DataUpdateEvent::DataUpdateEvent(const SystemState* systemState, const BPMN::Scope* scope, BPMNOS::number instance, Values values, Objects objects)
  : Event(nullptr)
  , systemState(systemState)
  , scope(scope)
  , instance(instance)
  , values(std::move(values))
  , objects(std::move(objects))
{
  if ( !scope ) {
    throw std::invalid_argument("DataUpdateEvent: no scope given for the data of instance '" + BPMNOS::to_string(instance,STRING) + "'");
  }
  for ( auto& [attribute, _] : this->values ) {
    validate(attribute, false);
  }
  for ( auto& [object, _] : this->objects ) {
    validate(object, true);
  }
}

void DataUpdateEvent::validate(const BPMNOS::Model::Attribute* attribute, bool isObject) const {
  if ( attribute->category != BPMNOS::Model::Attribute::Category::DATA ) {
    throw std::invalid_argument("DataUpdateEvent: '" + attribute->id + "' is no data or global attribute");
  }
  if ( attribute->isObject() != isObject ) {
    throw std::invalid_argument("DataUpdateEvent: '" + attribute->id + "' is " + ( isObject ? "no object" : "an object" ));
  }
  // the time is changed only by a clock tick, and the instance by nothing
  if ( !attribute->isObject() && attribute->isGlobal && attribute->index == BPMNOS::Model::ExtensionElements::Index::Time ) {
    throw std::invalid_argument("DataUpdateEvent: the time must not be updated");
  }
  if ( attribute->id == BPMNOS::Keyword::Instance ) {
    throw std::invalid_argument("DataUpdateEvent: the instance must not be updated");
  }
  if ( attribute->isImmutable ) {
    throw std::invalid_argument("DataUpdateEvent: '" + attribute->id + "' is immutable");
  }
  if ( !scope ) {
    if ( !attribute->isGlobal ) {
      throw std::invalid_argument("DataUpdateEvent: '" + attribute->id + "' is no global attribute");
    }
    return;
  }
  // the attributes a scope declares are the data of the data objects within it
  auto extensionElements = scope->extensionElements ? scope->extensionElements->as<BPMNOS::Model::ExtensionElements>() : nullptr;
  auto declares = [attribute](const std::vector< std::unique_ptr<BPMNOS::Model::Attribute> >& declared) {
    return std::ranges::any_of(declared, [attribute](const std::unique_ptr<BPMNOS::Model::Attribute>& candidate) { return candidate.get() == attribute; });
  };
  if ( !extensionElements || !( declares(extensionElements->data) || declares(extensionElements->dataObjects) ) ) {
    throw std::invalid_argument("DataUpdateEvent: '" + attribute->id + "' is no data attribute of '" + scope->id + "'");
  }
}

void DataUpdateEvent::processBy(Engine* engine) const {
  engine->process(this);
}

StateMachine* DataUpdateEvent::owner() const {
  if ( !scope ) {
    return systemState->stateMachine.get();
  }
  // the archive holds the state machines of the process instances, of the instances of multi-instance activities and
  // of the instances of non-interrupting event subprocesses, each by its identifier; any other state machine shares
  // the identifier of one of them and is found below it
  auto it = systemState->archive.find( (long unsigned int)instance );
  if ( it == systemState->archive.end() ) {
    return nullptr;
  }
  auto archived = it->second.lock();
  if ( !archived ) {
    return nullptr;
  }
  return find(archived.get());
}

StateMachine* DataUpdateEvent::find(StateMachine* stateMachine) const {
  if ( stateMachine->scope == scope ) {
    return stateMachine;
  }
  // a subprocess and an interrupting event subprocess share the identifier of the state machine they belong to, whereas
  // every state machine with an identifier of its own is archived and reached from the archive
  for ( auto& token : stateMachine->tokens ) {
    if ( token->owned && token->owned->instance == instance ) {
      if ( auto found = find(token->owned.get()) ) {
        return found;
      }
    }
  }
  if ( auto& interrupting = stateMachine->interruptingEventSubProcess; interrupting && interrupting->instance == instance ) {
    return find(interrupting.get());
  }
  return nullptr;
}

bool DataUpdateEvent::expired() const {
  return !owner();
}

nlohmann::ordered_json DataUpdateEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "dataupdate";
  if ( scope ) {
    jsonObject["scopeId"] = scope->id;
    jsonObject["instanceId"] = BPMNOS::to_string(instance,STRING);
  }

  // a scalar attribute is rendered as a value of its type and an object as JSON, as for a token
  auto valueOf = [](const BPMNOS::Value& value, ValueType type) -> nlohmann::ordered_json {
    if ( !value.has_value() ) {
      return nullptr;
    }
    switch ( type ) {
      case BOOLEAN:
        return (bool)value.value();
      case INTEGER:
        return (int)value.value();
      case DECIMAL:
        return (double)value.value();
      case STRING:
        return BPMNOS::to_string(value.value(),type);
    }
    return nullptr;
  };
  auto updates = nlohmann::ordered_json::object();
  for ( auto& [attribute, value] : values ) {
    updates[attribute->name] = valueOf(value, std::get<BPMNOS::ValueType>(attribute->type));
  }
  for ( auto& [object, value] : objects ) {
    updates[object->name] = value ? value->jsonify() : nlohmann::ordered_json(nullptr);
  }
  jsonObject[ scope ? "data" : "globals" ] = std::move(updates);
  if ( expired() ) {
    jsonObject["expired"] = true;
  }

  return jsonObject;
}
