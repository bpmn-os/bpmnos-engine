#include "Scenario.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Model;

Scenario::Scenario(const Model* model, const DataInput& attributes, unsigned int index)
  : index(index)
  , model(model)
  , attributes(attributes)
{
}

Scenario::Scenario(const Scenario& other, unsigned int index)
    : index(index)
    , model(other.model)
    , attributes(other.attributes)
{
  // Implement deep copy logic for the 'instances' map and its elements
  for (auto& [identifier, instance] : other.instances) {
    std::unordered_map< const Attribute*, Data > data;
    for ( auto& [attribute, attributeData] : instance.data ) {
      data[attribute] = attributeData;
    }
    instances[identifier] = {instance.process,instance.identifier,instance.instantiation,data};
  }
}

std::vector< const Scenario::InstanceData* > Scenario::getKnownInstances(BPMNOS::number time) const {
  std::vector< const Scenario::InstanceData* > knownInstances;

  for ( auto [id, instance] : instances ) {
    if ( instance.instantiation.realization && instance.instantiation.realization->disclosure <= time ) {
      knownInstances.push_back(&instance);
    }  
  }
  return knownInstances;
}

std::vector< const Scenario::InstanceData* > Scenario::getKnownInstantiations(BPMNOS::number time) const {
  std::vector< const Scenario::InstanceData* > knownInstantiatons;

  for ( auto [id, instance] : instances ) {
    if ( instance.instantiation.realization && instance.instantiation.realization->value == time ) {
      knownInstantiatons.push_back(&instance);
    }  
  }
  return knownInstantiatons;
}


bool Scenario::getKnownValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time) const {
  // get instance id from status
  std::string instanceId = stringRegistry[ (long unsigned int)status[Status::Index::Instance].value() ];
  auto& instance = instances.at(instanceId);

  Values newAttributes;
  for ( auto& attribute : node->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance.data.at(attribute.get());
    
    if ( data.realization && data.realization->disclosure > time ) {
      return false;
    }
    newAttributes.push_back( data.realization->value );
  }

  status.insert(status.end(), newAttributes.begin(), newAttributes.end());
  return true;
}

BPMNOS::Values Scenario::getKnownStatus(const Scenario::InstanceData* instance, BPMNOS::number time) const {
  BPMNOS::Values initalStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance->data.at(attribute.get());
    
    if ( data.realization && data.realization->disclosure > time ) {
      throw std::runtime_error("Scenario: cannot instantiate '" + instance->identifier + "' because attribute '"+ attribute->id +"' is not yet known at time " + BPMNOS::to_string(time,INTEGER) );
    }
    initalStatus.push_back( data.realization->value );
  }
  return initalStatus;
}


std::vector< const Scenario::InstanceData* > Scenario::getAssumedInstances(BPMNOS::number time) const {
  std::vector< const Scenario::InstanceData* > assumedInstances;

  for ( auto [id, instance] : instances ) {
    if ( instance.instantiation.realization && instance.instantiation.realization->disclosure <= time ) {
      assumedInstances.push_back(&instance);
    }  
    else if ( instance.instantiation.anticipations.size() && instance.instantiation.anticipations.front().disclosure <= time ) {
      assumedInstances.push_back(&instance);
    }  
  }
  return assumedInstances;
}

std::vector< const Scenario::InstanceData* > Scenario::getAssumedInstantiations(BPMNOS::number time) const {
  std::vector< const Scenario::InstanceData* > assumedInstantiatons;

  for ( auto [id, instance] : instances ) {
    if ( instance.instantiation.realization && instance.instantiation.realization->value == time ) {
      assumedInstantiatons.push_back(&instance);
    }  
    else if ( instance.instantiation.anticipations.size() && getLatestDisclosure(instance.instantiation.anticipations,time).value == time ) {
      assumedInstantiatons.push_back(&instance);
    }  
  }
  return assumedInstantiatons;
}

void Scenario::getAssumedValues(const BPMN::FlowNode* node, Values& status,  BPMNOS::number time) const {
  // get instance id from status
  std::string instanceId = stringRegistry[ (long unsigned int)status[Status::Index::Instance].value() ];
  auto& instance = instances.at(instanceId);

  Values newAttributes;
  for ( auto& attribute : node->extensionElements->as<const Status>()->attributes ) {
    auto& data = instance.data.at(attribute.get());
    if ( data.realization && data.realization->disclosure <= time ) {
      // add the realized value
      newAttributes.push_back( data.realization->value );
    }
    else if ( data.anticipations.size() && data.anticipations.front().disclosure <= time ) {
      // add the currently anticipated value
      newAttributes.push_back( getLatestDisclosure(data.anticipations,time).value );
    }
    else {
      // add undefined value
      newAttributes.push_back( std::nullopt );
    }
  }

  status.insert(status.end(), newAttributes.begin(), newAttributes.end());
}

const Scenario::Disclosure& Scenario::getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, BPMNOS::number time) const {
return data.front();
  // find the first element with a disclosure time larger than the given time
  auto it = std::upper_bound(data.begin(), data.end(), time,
    [](BPMNOS::number t, const Disclosure& element) -> bool { return (element.disclosure > t); }
  ); 
  if (it == data.begin()) {
    throw std::runtime_error("Scenario: illegal request for latest disclosure");
  }
  // the preceding element is the last element with a disclosure time smaller or equal to the given time
  it--;
  return *it;
}

void Scenario::addInstance(const BPMN::Process* process, const std::string& identifier, Scenario::Data instantiation ) {
  // add instance
  instances[identifier] = {process,identifier,instantiation,{}};

  auto& instance = instances[identifier];
  // initialize all attribute data
  for ( auto& [id,attribute] : attributes.at(process) ) {
    // anticipation is set to the default attribute value
    instance.data[attribute] = { {{0,attribute->value}}, std::nullopt };
  }

}

void Scenario::removeAnticipatedInstance(const std::string& identifier) {
  auto& instance = instances[identifier];
  if ( instance.instantiation.realization ) {
    throw std::runtime_error("Scenario: illegal removal of instance '" + identifier + "'with known realization");
  }
  instances.erase(identifier);
}

Scenario::Data& Scenario::getInstantiationData(std::string instanceId) {
  auto& instance = instances[instanceId];
  return instance.instantiation;
}

Scenario::Data& Scenario::getAttributeData(std::string instanceId, const Attribute* attribute) {
  auto& instance = instances[instanceId];
  return instance.data[attribute];
}

void Scenario::addAnticipation( Scenario::Data& data, Scenario::Disclosure anticipation ) {
  if ( data.anticipations.back().disclosure >= anticipation.disclosure ) {
    throw std::runtime_error("Scenario: disclosures must be provided in strictly increasing order");
  }
  data.anticipations.push_back(anticipation);
}

void Scenario::setRealization( Scenario::Data& data, Scenario::Disclosure realization ) {
  data.realization = realization;
}
