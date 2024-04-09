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
          if ( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
            return true;
          }
        }
        return false;
      }
    );

    attributes[process.get()] = {};
    // add all attributes of process
    for ( auto& node : nodes ) {
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      // add all status attributes
      for ( auto& attribute : extensionElements->attributes ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
      // add all data attributes
      for ( auto& attribute : extensionElements->data ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
      
      // add all guiding attributes
      if ( extensionElements->entryGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->entryGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->exitGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->exitGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->choiceGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->choiceGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->messageDeliveryGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->messageDeliveryGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
    }
  }

}

DataProvider::~DataProvider() {}

const Model& DataProvider::getModel() const {
  return *model;
}
