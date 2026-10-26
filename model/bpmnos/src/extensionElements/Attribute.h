#ifndef BPMNOS_Model_Attribute_H
#define BPMNOS_Model_Attribute_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/utility/src/Schema.h"
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
  /// @brief Constructor for the content object of a message or signal, a status object named after the message or
  /// signal and of its type, which the operators of the node stating it write or read.
  Attribute(std::string& name, const Schema& schema, AttributeRegistry& attributeRegistry);
  XML::bpmnos::tAttribute* element; ///< The element declaring the attribute, nullptr for the content object of a message or signal

  Category category;
  size_t index; ///< Index of attribute (is automatically set by attribute registry).
  /// Position of the attribute among the attributes, scalar and object alike, in the order of their declaration,
  /// outer scopes first (set by the attribute registry), by which an outside view orders them
  size_t declaration = 0;

  std::string& id;
  Schema type; ///< The attribute type
  /// How the model initialises the attribute: std::monostate if it gives no initial value, the index in the object
  /// registry of the literal an object attribute is initialised with, or the compiled assignment in every other case.
  std::variant<std::monostate, size_t, std::unique_ptr<const Expression>> initialization;
  const std::string name;

  /// Returns true if the attribute is an object attribute, i.e. its type is an array type or a struct type, numbered
  /// separately from the other attributes.
  bool isObject() const { return !std::holds_alternative<ValueType>(type); }
 
  double weight; ///< Weight to be used for objective (assuming maximization). 

  bool isImmutable; ///< Flag indicating whether attribute value may be changed by operator, choice, or intermediate catch event. 
  bool isGlobal = false; ///< Flag indicating whether the attribute is global, i.e. declared by a data store, whether it is a scalar attribute or an object.
private:
  std::variant<std::monostate, size_t, std::unique_ptr<const Expression>> getInitialization(std::string& input, AttributeRegistry& attributeRegistry);
  Schema getType(const std::string& input);
  std::string getName(std::string& input);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Attribute_H
