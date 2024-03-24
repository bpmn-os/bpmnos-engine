#ifndef BPMNOS_Model_Attribute_H
#define BPMNOS_Model_Attribute_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class Attribute;
class Parameter;

typedef std::unordered_map<std::string, Attribute*> AttributeMap;

class Attribute {
public:
  Attribute(XML::bpmnos::tAttribute* attribute, AttributeMap& statusAttributes);
  XML::bpmnos::tAttribute* element;

  std::size_t index; ///< Index of attribute in status.

  std::string& id;
  std::string& name;

  ValueType type;
  std::unique_ptr<Parameter> collection; ///< Parameter for value initialization for multi-instance activities.
  std::optional<number> value;
 
  double weight; ///< Weight to be used for objective (assuming maximization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed. 
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Attribute_H
