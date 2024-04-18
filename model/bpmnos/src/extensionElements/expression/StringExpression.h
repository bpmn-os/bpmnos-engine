#ifndef BPMNOS_Model_StringExpression_H
#define BPMNOS_Model_StringExpression_H

#include <vector>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class allowing to compare strings.
 **/
class StringExpression : public Expression {
public:
  StringExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
 
  enum class Type { EQUAL, NOTEQUAL };
  Type type;
  Attribute* attribute;
  std::variant<const Attribute*, BPMNOS::number > rhs;

/**
 * @brief Checks whether the attribute matches a given string.
 */
  template <typename DataType>
  std::optional<BPMNOS::number> _execute(const BPMNOS::Values& status, const DataType& data) const;

  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::Values& data) const override { return _execute(status,data); };
  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::SharedValues& data) const override { return _execute(status,data); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StringExpression_H
