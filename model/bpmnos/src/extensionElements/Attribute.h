#ifndef BPMNOS_Model_Attribute_H
#define BPMNOS_Model_Attribute_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "Schema.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class Attribute;
class Parameter;
class Expression;

class AttributeRegistry;

class Attribute {
public:
  enum class Category { STATUS, DATA };
  Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry);
  XML::bpmnos::tAttribute* element;

  Category category;
  size_t index; ///< Index of attribute (is automatically set by attribute registry).

  std::string& id;
  /// The shape of an object attribute, stated by its type; nullptr for a scalar attribute.
  std::unique_ptr<const Schema> schema;
  /// The index in the object registry of the literal an object is initialised with, if the model gives one.
  std::optional<size_t> initialObject;
  std::unique_ptr<const Expression> expression;
  const std::string name;

  /// The type of a scalar attribute; for an object, the scalar type of its base, or decimal for an object base.
  ValueType type;
  /// Returns true if the attribute is an object, i.e. an array or a structured value, numbered separately from
  /// the scalar attributes.
  bool isObject() const { return schema != nullptr; }
 
  double weight; ///< Weight to be used for objective (assuming maximization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed by operator, choice, or intermediate catch event. 
private:
  std::unique_ptr<const Expression> getExpression(std::string& input, AttributeRegistry& attributeRegistry);
  std::unique_ptr<const Schema> getSchema(const std::string& input);
  std::string getName(std::string& input);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Attribute_H
