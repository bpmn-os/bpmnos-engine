#ifndef BPMNOS_Model_Enumeration_H
#define BPMNOS_Model_Enumeration_H

#include <vector>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a condition to determine whether an attribute value is element of a comma separated list of values.
 *
 * The list of values must be separated by comma and may contain strings within double quotes, 
 * attribute names, reserved keywords, and numbers. Strings must not contain commas. 
 **/
class Enumeration : public Expression {
public:
  Enumeration(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
  enum class Type { IN, NOTIN };
  Type type;
  static constexpr std::string IN = std::string("in");
  static constexpr std::string NOTIN = std::string("not in");
  Attribute* attribute;
  std::variant<const Attribute*, BPMNOS::number> collection;

/**
 * @brief Checks whether the attribute value is in the collection of values.
 */
  template <typename DataType>
  std::optional<BPMNOS::number> _execute(const BPMNOS::Values& status, const DataType& data) const;

  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::Values& data) const override { return _execute(status,data); };
  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::SharedValues& data) const override { return _execute(status,data); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Enumeration_H
