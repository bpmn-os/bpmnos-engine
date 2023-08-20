#include "DataProvider.h"

using namespace BPMNOS;

DataProvider::DataProvider(const std::string& modelFile)
  : model(std::make_unique<Model>(modelFile))
{
}

DataProvider::~DataProvider() {}

const Model& DataProvider::getModel() const {
  return *model;
}

const std::unordered_map<std::string, std::unique_ptr< InstanceData > >& DataProvider::getInstances() const {
  return instances;
}

void DataProvider::appendActualValues( const InstanceData* instance, const BPMN::Node* node, Values& values ) const {
  auto status = node->extensionElements->as<Status>();
  for ( auto& attribute : status->attributes ) {
    std::optional<number> value = instance->getActualValue(attribute.get());
    if ( value ) {
      values.push_back( value.value() );
    }
    else {
      values.push_back(std::nullopt);
    }
  }
}

void DataProvider::appendPredictedValues( 
  [[maybe_unused]] const InstanceData* instance,
  [[maybe_unused]] const BPMN::Node* node, 
  [[maybe_unused]] Values& values )
  const {}

void DataProvider::appendAssumedValues(
  [[maybe_unused]] const InstanceData* instance,
  [[maybe_unused]] const BPMN::Node* node, 
  [[maybe_unused]] Values& values )
  const {}



