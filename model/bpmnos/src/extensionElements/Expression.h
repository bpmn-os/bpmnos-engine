#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <limex.h>
#include <set>

#include "Attribute.h"
#include "AttributeRegistry.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a mathematical expression.
 **/
class Expression {
public:
  enum class Type { ASSIGN, UNASSIGN, IS_NULL, IS_NOT_NULL, OTHER };
  Expression(const std::string expression, const AttributeRegistry& attributeRegistry, bool newTarget = false);
  const AttributeRegistry& attributeRegistry;
  const std::string expression;
  const LIMEX::Expression<double> compiled;
  const Type type;
  std::optional<const Attribute*> target;
  std::set<const Attribute*> inputs; ///< Vector containing all input attributes and collections used by the expression.
  std::vector<const Attribute*> variables; ///< Vector containing all input attributes used by the expression.
  std::vector<const Attribute*> collections; ///< Vector containing all input collections used by the expression.
  const Attribute* isAttribute() const; ///< Returns pointer to the attribute if and only if expression contains nothing else
  template <typename DataType>
  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;
private:
  LIMEX::Expression<double> getExpression(const std::string& input) const;
  Type getType() const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
