#include "Scenario.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <limits>

using namespace BPMNOS::Model;

Scenario::Scenario(const Model* model, BPMNOS::number inception, BPMNOS::number completion, const DataInput& attributes, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap, unsigned int index)
  : index(index)
  , model(model)
  , attributes(attributes)
  , inception(inception)
  , completion(completion)
{
  globals.resize(model->attributes.size());
  for ( auto& [ attribute, value ] : globalValueMap ) {
    globals[attribute->index] = value;
  }
}

Scenario::Scenario(const Scenario& other, unsigned int index)
  : index(index)
  , model(other.model)
  , attributes(other.attributes)
  , inception(other.inception)
  , completion(other.completion)
{
  // Implement deep copy logic for the 'instances' map and its elements
  for (auto& [identifier, instance] : other.instances) {
    std::unordered_map< const Attribute*, Data > data;
    for ( auto& [attribute, attributeData] : instance.data ) {
      data[attribute] = attributeData;
    }
    instances[(long unsigned int)identifier] = {instance.process,instance.id,instance.instantiation,data};
  }
}

void Scenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, Scenario::Data instantiation ) {
  // add instance
  instances[(long unsigned int)instanceId] = {process,(long unsigned int)instanceId,instantiation,{}};
  auto& instanceData = instances[(long unsigned int)instanceId];
  // initialize all attribute data
  assert( attributes.contains(process) );
  for ( auto& [id,attribute] : attributes.at(process) ) {
    instanceData.data[attribute] = { {}, std::nullopt };
  }

}

void Scenario::removeAnticipatedInstance(const BPMNOS::number instanceId) {
  auto& instanceData = instances[(long unsigned int)instanceId];
  if ( instanceData.instantiation.realization ) {
    throw std::runtime_error("Scenario: illegal removal of instance '" + BPMNOS::to_string(instanceId,STRING) + "'with known realization");
  }
  instances.erase((long unsigned int)instanceId);
}

void Scenario::addAnticipation( Scenario::Data& data, Scenario::Disclosure anticipation ) {
  if ( data.anticipations.size() && data.anticipations.back().disclosure >= anticipation.disclosure ) {
    throw std::runtime_error("Scenario: disclosures must be provided in strictly increasing order");
  }
  data.anticipations.push_back(anticipation);
}

void Scenario::setRealization( Scenario::Data& data, Scenario::Disclosure realization ) {
  data.realization = realization;
}

const Model* Scenario::getModel() const {
  return model;
}

BPMNOS::number Scenario::getInception() const {
  return inception;
}

bool Scenario::isCompleted(const BPMNOS::number currentTime) const {
  return currentTime > completion;
}


std::vector< const Scenario::InstanceData* > Scenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > knownInstances;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->disclosure <= currentTime
         && instance.instantiation.realization->value.value() <= currentTime
    ) {
      knownInstances.push_back(&instance);
    }
  }
  return knownInstances;
}

std::vector< const Scenario::InstanceData* > Scenario::getKnownInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > knownInstances;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
      && instance.instantiation.realization->disclosure <= currentTime
    ) {
      knownInstances.push_back(&instance);
    }
  }
  return knownInstances;
}

std::vector< const Scenario::InstanceData* > Scenario::getAnticipatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > anticipatedInstances;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->disclosure <= currentTime
    ) {
      // instance is already known
      continue;
    }
    else if ( instance.instantiation.anticipations.size()
              && instance.instantiation.anticipations.front().disclosure <= currentTime
    ) {
      anticipatedInstances.push_back(&instance);
    }
  }
  return anticipatedInstances;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > Scenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > instantiations;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->value.value() == currentTime
    ) {
      // return instantiations known to occurr at current time
      instantiations.push_back({instance.process, getKnownInitialStatus(&instance,currentTime), getKnownInitialData(&instance,currentTime)});
    }
  }
  return instantiations;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > Scenario::getAnticipatedInstantiations(const BPMNOS::number currentTime, const BPMNOS::number assumedTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > instantiations;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->disclosure <= currentTime
         && instance.instantiation.realization->value.value() == assumedTime
    ) {
      // return known instantiations if realization is already disclosed at current time
      instantiations.push_back({instance.process, getKnownInitialStatus(&instance,currentTime), getKnownInitialData(&instance,currentTime)});
    }
    else if ( instance.instantiation.anticipations.size()
              && instance.instantiation.anticipations.front().disclosure <= currentTime
              && getLatestDisclosure(instance.instantiation.anticipations,currentTime).value.value() == assumedTime
    ) {
      // return anticipated instantiations if anticipation is already disclosed at current time
     instantiations.push_back({ instance.process, getAnticipatedInitialStatus(&instance, currentTime), getAnticipatedInitialData(&instance, currentTime) });
    }

  }
  return instantiations;
}

BPMNOS::Values Scenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initalStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    assert( instance->data.contains(attribute.get()) );
    auto& data = instance->data.at(attribute.get());
    if ( data.realization.has_value() && data.realization.value().disclosure > currentTime ) {
        throw std::runtime_error("Scenario: cannot instantiate '" + BPMNOS::to_string(instance->id,STRING) + "' because data attribute '"+ attribute->id +"' is not yet known at time " + BPMNOS::to_string(currentTime,INTEGER) );
    }

    initalStatus.push_back( getKnownValue(instance,attribute.get(),currentTime) );
  }
  return initalStatus;
}

BPMNOS::Values Scenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initalData;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    assert( instance->data.contains(attribute.get()) );
    auto& data = instance->data.at(attribute.get());
    if ( data.realization.has_value() && data.realization.value().disclosure > currentTime ) {
        throw std::runtime_error("Scenario: cannot instantiate '" + BPMNOS::to_string(instance->id,STRING) + "' because data attribute '"+ attribute->id +"' is not yet known at time " + BPMNOS::to_string(currentTime,INTEGER) );
    }

    initalData.push_back( getKnownValue(instance,attribute.get(),currentTime) );
  }
  return initalData;
}

std::optional<BPMNOS::number> Scenario::getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // value is obtained by an assignment
    // collect variable values
    std::vector< double > variableValues;
    for ( auto input : attribute->expression->variables ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;        
      }
      auto value = getKnownValue(instance, input, currentTime);
      if ( !value.has_value() ) {
        // return nullopt because required attribute value is not given
        return std::nullopt;        
      }
      variableValues.push_back( (double)value.value() );
    }

    // collect values of all variables in collection
    std::vector< std::vector< double > > collectionValues;
    for ( auto input : attribute->expression->collections ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;        
      }
      collectionValues.push_back( {} );
      auto collection = getKnownValue(instance, input, currentTime);
      if ( !collection.has_value() ) {
        // return nullopt because required collection is not given
        return std::nullopt;
      }
      for ( auto value : collectionRegistry[(size_t)collection.value()] ) {
        collectionValues.back().push_back( value );
      }
    }

    return number(attribute->expression->compiled.evaluate(variableValues,collectionValues));      
  }
  else {
    auto& data = instance->data.at(attribute);

    if ( data.realization.has_value() ) {
      auto realization = data.realization.value();
      if ( realization.disclosure > currentTime ) {
        // value not yet disclosed
        return std::nullopt;
        }
      else {
        return realization.value;
      }
    }
  }
  
  // value will never be provided
  return std::nullopt;
}

std::optional<BPMNOS::number> Scenario::getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);
  return getKnownValue(&instanceData,attribute,currentTime);
}


std::optional<BPMNOS::Values> Scenario::getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    assert( instanceData.data.contains(attribute.get()) );
    values.push_back( getKnownValue(&instanceData,attribute.get(),currentTime) );
  }

  return values;
}


std::optional<BPMNOS::Values> Scenario::getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    assert( instanceData.data.contains(attribute.get()) );
    values.push_back( getKnownValue(&instanceData,attribute.get(),currentTime) );
  }

  return values;
}

std::optional<BPMNOS::number> Scenario::getAnticipatedValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // value is obtained by an assignment
    // collect variable values
    std::vector< double > variableValues;
    for ( auto input : attribute->expression->variables ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;        
      }
      auto value = getAnticipatedValue(instance, input, currentTime);
      if ( !value.has_value() ) {
        // return nullopt because required attribute value is not given
        return std::nullopt;        
      }
      variableValues.push_back( (double)value.value() );
    }

    // collect values of all variables in collection
    std::vector< std::vector< double > > collectionValues;
    for ( auto input : attribute->expression->collections ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;        
      }
      collectionValues.push_back( {} );
      auto collection = getAnticipatedValue(instance, input, currentTime);
      if ( !collection.has_value() ) {
        // return nullopt because required collection is not given
        return std::nullopt;
      }
      for ( auto value : collectionRegistry[(size_t)collection.value()] ) {
        collectionValues.back().push_back( value );
      }
    }

    return number(attribute->expression->compiled.evaluate(variableValues,collectionValues));      
  }
  else {
    auto& data = instance->data.at(attribute);
    if ( data.realization && data.realization->disclosure <= currentTime ) {
      // return the realized value
      return data.realization->value;
    }
    else if ( data.anticipations.size() && data.anticipations.front().disclosure <= currentTime ) {
      // return the currently anticipated value
      return getLatestDisclosure(data.anticipations,currentTime).value;
    }
  }
  // return undefined value
  return std::nullopt;
}

std::optional<BPMNOS::number> Scenario::getAnticipatedValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);
  return getAnticipatedValue(&instanceData,attribute,currentTime);
}

BPMNOS::Values Scenario::getAnticipatedInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initalStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    assert( instance->data.contains(attribute.get()) );
    initalStatus.push_back( getAnticipatedValue(instance,attribute.get(),currentTime) );
  }
  return initalStatus;
}

BPMNOS::Values Scenario::getAnticipatedInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initalData;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    assert( instance->data.contains(attribute.get()) );
    initalData.push_back( getAnticipatedValue(instance,attribute.get(),currentTime) );
  }
  return initalData;
}


BPMNOS::Values Scenario::getAnticipatedValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);


  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    assert( instanceData.data.contains(attribute.get()) );
    values.push_back( getAnticipatedValue(&instanceData,attribute.get(),currentTime) );
  }
  return values;
}

BPMNOS::Values Scenario::getAnticipatedData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    values.push_back( getAnticipatedValue(&instanceData,attribute.get(),currentTime) );
  }
  return values;
}

const Scenario::Disclosure& Scenario::getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, const BPMNOS::number currentTime) const {
  // find the first element with a disclosure time larger than the given time
  auto it = std::upper_bound(data.begin(), data.end(), currentTime,
    [](BPMNOS::number t, const Disclosure& element) -> bool { return (element.disclosure > t); }
  );
  if (it == data.begin()) {
    throw std::runtime_error("Scenario: illegal request for latest disclosure");
  }
  // the preceding element is the last element with a disclosure time smaller or equal to the given time
  it--;
  return *it;
}


Scenario::Data& Scenario::getInstantiationData(const BPMNOS::number instanceId) {
  auto& instanceData = instances[(size_t)instanceId];
  return instanceData.instantiation;
}

Scenario::Data& Scenario::getAttributeData(const BPMNOS::number instanceId, const Attribute* attribute) {
  auto& instanceData = instances[(size_t)instanceId];
  return instanceData.data[attribute];
}

