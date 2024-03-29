#include "DataProvider.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

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
          if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
            return extensionElements->attributes.size() || extensionElements->data.size();
          }
        }
        return false;
      }
    );

    attributes[process.get()] = {};
    // add all attributes of process
    for ( auto& node : nodes ) {
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      for ( auto& attribute : extensionElements->attributes ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
      for ( auto& attribute : extensionElements->data ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
    }
  }

}

DataProvider::~DataProvider() {}

const Model& DataProvider::getModel() const {
  return *model;
}
