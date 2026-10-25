#include "InstanceValues.h"
#include "DataProvider.h"
#include "InstanceDataReader.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstanceValues::InstanceValues(
  const DataProvider& dataProvider,
  const BPMN::Node* node,
  BPMNOS::number time,
  BPMNOS::Status inheritedStatus,
  const BPMNOS::SharedData& inheritedData,
  std::optional<BPMNOS::number> instance,
  const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& givenValues,
  const std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>& givenObjects
)
  : status(std::move(inheritedStatus))
  , data(node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->data.size())
  , dataProvider(dataProvider)
  , attributeRegistry(node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry)
  , identity(instance)
{
  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto instanceIndex = dataProvider.getModel()->instanceIndex;

  // every slot is laid out before any value is written, so that every index of the registry is valid
  assert( status.attributes.size() + extensionElements->attributes.size() == attributeRegistry.statusAttributes.size() );
  status.attributes.resize( attributeRegistry.statusAttributes.size() );
  auto objectOf = [&](const BPMNOS::Model::Attribute* object) -> std::shared_ptr<const BPMNOS::Object> {
    if ( auto it = givenObjects.find(object); it != givenObjects.end() ) {
      return it->second;
    }
    return dataProvider.getDefaultObject(object);
  };
  status.objects.reserve(status.objects.size() + extensionElements->statusObjects.size());
  for ( auto& object : extensionElements->statusObjects ) {
    status.objects.push_back( objectOf(object.get()) );
  }
  data.objects.reserve(data.objects.size() + extensionElements->dataObjects.size());
  for ( auto& object : extensionElements->dataObjects ) {
    data.objects.push_back( objectOf(object.get()) );
  }
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = time;

  // the identity of a process instance is its first data attribute, that of a non-interrupting event
  // subprocess instance is referred to at the position of the instance in the data chain
  if ( instance.has_value() && node->represents<BPMN::Process>() ) {
    data.attributes[BPMNOS::Model::ExtensionElements::Position::Instance] = instance;
  }
  chain = BPMNOS::SharedData(inheritedData, data);
  if ( instance.has_value() && !node->represents<BPMN::Process>() ) {
    chain.attributes[instanceIndex] = std::ref(identity);
  }

  // the own attributes are given their initial values in declaration order, the model deciding the source
  auto initialise = [&](const BPMNOS::Model::Attribute* attribute) {
    if ( auto expression = std::get_if< std::unique_ptr<const BPMNOS::Model::Expression> >(&attribute->initialization) ) {
      attributeRegistry.setValue( attribute, status, chain, BPMNOS::to_value( (*expression)->execute(status, chain) ) );
    }
    else if ( auto it = givenValues.find(attribute); it != givenValues.end() ) {
      attributeRegistry.setValue( attribute, status, chain, it->second );
    }
  };
  for ( auto& attribute : extensionElements->attributes ) {
    // the timestamp is given by the time at which the instance comes into being
    if ( attribute->index != BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      initialise(attribute.get());
    }
  }
  for ( auto& attribute : extensionElements->data ) {
    // the instance is given by its identity
    if ( attribute->id != BPMNOS::Keyword::Instance ) {
      initialise(attribute.get());
    }
  }
  for ( auto objects : { &extensionElements->statusObjects, &extensionElements->dataObjects } ) {
    for ( auto& object : *objects ) {
      if ( auto expression = std::get_if< std::unique_ptr<const BPMNOS::Model::Expression> >(&object->initialization) ) {
        (*expression)->writeToObject(status, chain, dataProvider);
      }
    }
  }
}

void InstanceValues::assign(const BPMNOS::Model::Expression& expression) {
  if ( expression.writesToObject() ) {
    expression.writeToObject(status, chain, dataProvider);
    return;
  }
  auto target = expression.target.value();
  if ( auto value = expression.execute(status, chain); value.has_value() ) {
    attributeRegistry.setValue( target, status, chain, BPMNOS::cast(value.value(), std::get<BPMNOS::ValueType>(target->type)) );
  }
}
