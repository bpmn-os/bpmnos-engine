#ifndef BPMNOS_Attribute_H
#define BPMNOS_Attribute_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "xml/bpmnos/tAttribute.h"

namespace BPMNOS {

class Attribute;

typedef std::unordered_map<std::string, Attribute*> AttributeMap;

class Attribute {
public:
  Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap);
  XML::bpmnos::tAttribute* element;

  std::size_t index; ///< Index of attribute in status.

  std::string& id;
  std::string& name;

  enum Type { STRING, BOOLEAN, INTEGER, DECIMAL };
  Type type;

  double weight; ///< Weight to be used for objective (assuming minimization). 
};

} // namespace BPMNOS

#endif // BPMNOS_Attribute_H
