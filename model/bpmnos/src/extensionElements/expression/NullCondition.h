#ifndef BPMNOS_Model_NullCondition_H
#define BPMNOS_Model_NullCondition_H

#include <vector>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a condition to determine whether an attribute is defined or not.
 **/
class NullCondition : public Expression {
public:
  NullCondition(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
  enum class Type { ISNULL, NOTNULL };
  Type type;
  Attribute* attribute;

/**
 * @brief Checks whether the attribute is undefined or not.
 *
 * A null condition must have
 * - an attribute name on the l.h.s.
 * - a comparison operator "==" or "!=" 
 * - the @ref Keyword::Undefined on the r.h.s.
 */
  template <typename DataType>
  std::optional<BPMNOS::number> _execute(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;

  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const override { return _execute(status,data,globals); };
  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const override { return _execute(status,data,globals); };
private:
  void parse(const std::string& comparisonOperator);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_NullCondition_H
