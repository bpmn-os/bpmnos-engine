#include "InstanceData.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Model;

InstanceData::InstanceData(const BPMN::Process* process, const std::string& id) : process(process), id(id) {
  // get all nodes with attribute definition
  std::vector< const BPMN::Node* > nodes = process->find_all(
    [](const BPMN::Node* node) {
      if ( node->extensionElements ) {
        if ( auto status = node->extensionElements->represents<const Status>(); status ) {
          return !status->attributes.empty();
        }
      }
      return false;
    }
  );
 
  for ( auto& node : nodes ) {
    auto status = node->extensionElements->as<const Status>();
    for ( auto& attribute : status->attributes ) {
      attributes[attribute->id] = attribute.get();

      // add std::nullopt to actualValues for all attributes
      actualValues[attribute.get()] = std::nullopt;
      // add std::nullopt or value given in model to defaultValues for all attributes
      defaultValues[attribute.get()] = attribute->value;
    }
  }
}

std::optional<BPMNOS::number> InstanceData::getActualValue(const Attribute* attribute) const {
  return actualValues.at(attribute);
}

const Attribute* InstanceData::getAttributeById(const std::string& id) const {
  return attributes.at(id);
}

