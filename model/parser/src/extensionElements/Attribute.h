#ifndef BPMNOS_Attribute_H
#define BPMNOS_Attribute_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <bpmn++.h>
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class Attribute;

typedef std::unordered_map<std::string, Attribute*> AttributeMap;

class Attribute {
public:
  Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap);
  XML::bpmnos::tAttribute* element;

  std::size_t index; ///< Index of attribute in status.

  std::string& id;
  std::string& name;

  ValueType type;
  std::optional<number> value;
 
  double weight; ///< Weight to be used for objective (assuming minimization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed. 
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Attribute_H
