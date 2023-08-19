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
