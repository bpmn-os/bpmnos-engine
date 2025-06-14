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
class Expression;

class AttributeRegistry;

class Attribute {
public:
  enum class Category { STATUS, DATA, GLOBAL };
  Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry);
  XML::bpmnos::tAttribute* element;

  Category category;
  size_t index; ///< Index of attribute (is automatically set by attribute registry).

  std::string& id;
  std::unique_ptr<const Expression> expression;
  const std::string name; ///< Name of attribute and optional initial assignment.

  ValueType type;
 
  double weight; ///< Weight to be used for objective (assuming maximization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed by operator, choice, or intermediate catch event. 
private:
  std::unique_ptr<const Expression> getExpression(std::string& input, AttributeRegistry& attributeRegistry);
  std::string getName(std::string& input);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Attribute_H
