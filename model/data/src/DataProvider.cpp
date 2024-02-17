#include "DataProvider.h"
#include "model/parser/src/extensionElements/ExtensionElements.h"

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
          if ( auto status = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>(); status ) {
            return !status->attributes.empty();
          }
        }
        return false;
      }
    );

    attributes[process.get()] = {};
    // add all attributes of process
    for ( auto& node : nodes ) {
      auto status = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
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
