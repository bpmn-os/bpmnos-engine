#ifndef BPMNOS_Model_ContentDefinition_H
#define BPMNOS_Model_ContentDefinition_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tContent.h"
#include "Attribute.h"
#include "AttributeRegistry.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class ContentDefinition {
public:
  ContentDefinition(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tContent* element;

  std::string& key;
  Attribute* attribute;
};

/// The definitions of the content of a message or signal, by their keys.
typedef std::unordered_map< std::string, std::unique_ptr<ContentDefinition> > ContentDefinitionMap;

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ContentDefinition_H
