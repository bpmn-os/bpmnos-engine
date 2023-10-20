#include "Scenario.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Status.h"
#include <limits>

using namespace BPMNOS::Model;

Scenario::Scenario(const Model* model, BPMNOS::number inception, BPMNOS::number completion, const DataInput& attributes, unsigned int index)
  : index(index)
  , model(model)
  , attributes(attributes)
  , inception(inception)
  , completion(completion)
{
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
    instances[identifier] = {instance.process,instance.id,instance.instantiation,data};
  }
}

void Scenario::addInstance(const BPMN::Process* process, const std::string& instanceId, Scenario::Data instantiation ) {
  // add instance
  instances[instanceId] = {process,instanceId,instantiation,{}};
  auto& instance = instances[instanceId];
  // initialize all attribute data
  for ( auto& [id,attribute] : attributes.at(process) ) {
    instance.data[attribute] = { {}, std::nullopt };
  }

}

void Scenario::removeAnticipatedInstance(const std::string& instanceId) {
  auto& instance = instances[instanceId];
  if ( instance.instantiation.realization ) {
    throw std::runtime_error("Scenario: illegal removal of instance '" + instanceId + "'with known realization");
  }
  instances.erase(instanceId);
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

BPMNOS::number Scenario::getInception() const {
  return inception;
}

bool Scenario::isCompleted(BPMNOS::number currentTime) const {
  return currentTime > completion;
}


std::vector< const Scenario::InstanceData* > Scenario::getCreatedInstances(BPMNOS::number currentTime) const {
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

std::vector< const Scenario::InstanceData* > Scenario::getKnownInstances(BPMNOS::number currentTime) const {
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

std::vector< const Scenario::InstanceData* > Scenario::getAnticipatedInstances(BPMNOS::number currentTime) const {
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

std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > Scenario::getCurrentInstantiations(BPMNOS::number currentTime) const {
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > instantiations;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->value.value() == currentTime
    ) {
      // return instantiations known to occurr at current time
      instantiations.push_back({instance.process,getKnownInitialStatus(&instance,currentTime)});
    }
  }
  return instantiations;
}

std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > Scenario::getAnticipatedInstantiations(BPMNOS::number currentTime, BPMNOS::number assumedTime) const {
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > instantiations;

  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.realization
         && instance.instantiation.realization->disclosure <= currentTime
         && instance.instantiation.realization->value.value() == assumedTime
    ) {
      // return known instantiations if realization is already disclosed at current time
      instantiations.push_back({instance.process,getKnownInitialStatus(&instance,currentTime)});
    }
    else if ( instance.instantiation.anticipations.size()
              && instance.instantiation.anticipations.front().disclosure <= currentTime
              && getLatestDisclosure(instance.instantiation.anticipations,currentTime).value.value() == assumedTime
    ) {
      // return anticipated instantiations if anticipation is already disclosed at current time
     instantiations.push_back({ instance.process, getAnticipatedInitialStatus(&instance, currentTime) });
    }

  }
  return instantiations;
}

BPMNOS::Values Scenario::getKnownInitialStatus(const Scenario::InstanceData* instance, BPMNOS::number currentTime) const {
  BPMNOS::Values initalStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance->data.at(attribute.get());

    if ( data.realization && data.realization->disclosure > currentTime ) {
      throw std::runtime_error("Scenario: cannot instantiate '" + instance->id + "' because attribute '"+ attribute->id +"' is not yet known at time " + BPMNOS::to_string(currentTime,INTEGER) );
    }
    initalStatus.push_back( data.realization->value );
  }
  return initalStatus;
}

std::optional<BPMNOS::Values> Scenario::getKnownValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number currentTime) const {
  // get instance id from status
  std::string instanceId = stringRegistry[ (long unsigned int)status[Status::Index::Instance].value() ];
  auto& instance = instances.at(instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance.data.at(attribute.get());

    if ( data.realization && data.realization->disclosure > currentTime ) {
      return std::nullopt;
    }
    values.push_back( data.realization->value );
  }

  return values;
}

BPMNOS::Values Scenario::getAnticipatedInitialStatus(const Scenario::InstanceData* instance, BPMNOS::number currentTime) const {
  BPMNOS::Values initalStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance->data.at(attribute.get());

    if ( data.realization
         && data.realization->disclosure <= currentTime
    ) {
      initalStatus.push_back( data.realization->value );
    }
    else if ( data.anticipations.size()
              && data.anticipations.front().disclosure <= currentTime
    ) {
      // add the currently anticipated value
      initalStatus.push_back( getLatestDisclosure(data.anticipations,currentTime).value );
    }
    else {
      // add undefined value
      initalStatus.push_back( std::nullopt );
    }
  }
  return initalStatus;
}


BPMNOS::Values Scenario::getAnticipatedValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number currentTime) const {
  // get instance id from status
  std::string instanceId = stringRegistry[ (long unsigned int)status[Status::Index::Instance].value() ];
  auto& instance = instances.at(instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance.data.at(attribute.get());
    if ( data.realization
         && data.realization->disclosure <= currentTime
    ) {
      // add the realized value
      values.push_back( data.realization->value );
    }
    else if ( data.anticipations.size()
              && data.anticipations.front().disclosure <= currentTime
    ) {
      // add the currently anticipated value
      values.push_back( getLatestDisclosure(data.anticipations,currentTime).value );
    }
    else {
      // add undefined value
      values.push_back( std::nullopt );
    }
  }
  return values;
}

const Scenario::Disclosure& Scenario::getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, BPMNOS::number currentTime) const {
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


Scenario::Data& Scenario::getInstantiationData(std::string instanceId) {
  auto& instance = instances[instanceId];
  return instance.instantiation;
}

Scenario::Data& Scenario::getAttributeData(std::string instanceId, const Attribute* attribute) {
  auto& instance = instances[instanceId];
  return instance.data[attribute];
}

