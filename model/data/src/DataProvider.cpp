#include "DataProvider.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Model;

DataProvider::DataProvider(const std::string& modelFile)
  : model(std::make_unique<Model>(modelFile))
{
  // determine all attributes of all processes
  for ( auto& process : model->processes ) {
    // get all nodes in process with attribute definition
    std::vector< BPMN::Node* > nodes = process->find_all(
      [](BPMN::Node* node) {
        if ( node->extensionElements ) {
          if ( auto status = node->extensionElements->represents<Status>(); status ) {
            return !status->attributes.empty();
          }
        }
        return false;
      }
    );

    attributes[process.get()] = {};
    // add all attributes of process
    for ( auto& node : nodes ) {
      auto status = node->extensionElements->as<Status>();
      for ( auto& attribute : status->attributes ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
    }
  }

}

DataProvider::~DataProvider() {}

const Model& DataProvider::getModel() const {
  return *model;
}

std::vector< const Scenario::InstanceData* > DataProvider::getKnownInstances(BPMNOS::number time, unsigned int scenarioId) const {
  if ( scenarioId >= scenarios.size() ) {
    throw std::runtime_error("DataProvider: illegal scenarioId " + std::to_string(scenarioId));
  }
  return scenarios[scenarioId]->getKnownInstances(time);
}

bool DataProvider::getKnownValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time, unsigned int scenarioId) const {
  if ( scenarioId >= scenarios.size() ) {
    throw std::runtime_error("DataProvider: illegal scenarioId " + std::to_string(scenarioId));
  }
  return scenarios[scenarioId]->getKnownValues(node,status,time);
}

std::vector< const Scenario::InstanceData* > DataProvider::getAssumedInstances(BPMNOS::number time, unsigned int scenarioId) const {
  if ( scenarioId >= scenarios.size() ) {
    throw std::runtime_error("DataProvider: illegal scenarioId " + std::to_string(scenarioId));
  }
  return scenarios[scenarioId]->getAssumedInstances(time);
}

void DataProvider::getAssumedValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time, unsigned int scenarioId) const {
  if ( scenarioId >= scenarios.size() ) {
    throw std::runtime_error("DataProvider: illegal scenarioId " + std::to_string(scenarioId));
  }
  return scenarios[scenarioId]->getAssumedValues(node,status,time);
}
