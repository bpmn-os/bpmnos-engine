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

class AttributeRegistry;

class Attribute {
public:
  enum class Category { STATUS, DATA };
private:
  Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category = Attribute::Category::STATUS);
public:
  Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry);
  XML::bpmnos::tAttribute* element;

  Category category;
  size_t index; ///< Index of attribute (is automatically set by attribute registry).

  std::string& id;
  std::string& name;

  ValueType type;
  std::unique_ptr<Parameter> collection; ///< Parameter for value initialization for multi-instance activities.
  std::optional<number> value;
 
  double weight; ///< Weight to be used for objective (assuming maximization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed by operator, choice, or intermediate catch event. // TODO: intermediate catch events 
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Attribute_H
