#ifndef BPMNOS_Model_LinearExpression_H
#define BPMNOS_Model_LinearExpression_H

#include <vector>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a linear expression to determine a value by applying
 * the expression to a value vector.
 **/
class LinearExpression : public Expression {
public:
  LinearExpression(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes);
 
  using NumericType = double;
  using Term = std::tuple<NumericType,Attribute*>;
  std::vector<Term> terms;
  enum class Type { DEFAULT, EQUAL, NOTEQUAL, GREATEROREQUAL, GREATERTHAN, LESSOREQUAL, LESSTHAN };
  Type type;

/**
 * @brief Executes the linear expression and returns the result.
 *
 * Determines a value based on a linear expression composed of terms.
 * Linear expressions must follow these rules:
 * - Terms are separated by operators "+" or "-".
 * - Each term may consist of: 
 *   - a coefficient represented by a number, Keyword::True, or Keyword::False,
 *   - an operator "*" or "/"
 *   - and a variable represented by an attribute name.
 * - The denominator of a division must be a coefficient.
 * - Alternatively a term may consist of only a coeffcient or only a variable. 
 * - If any of the variables in the expression is undefined, the result is set
 *   to `std::nullopt`.
 * - Keyword::True and Keyword::False are replaced by 1 and 0, respectively.
 *
 * Additionaly, boolean expression containing the comparison
 * operators "==", "!=", ">=", ">", "<=", "<" are supported.
 */
  std::optional<BPMNOS::number> execute(const Values& values) const override;
  
  std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > getBounds(const Attribute* attribute, const Values& values) const override;

private:
  void parse(std::string expressionString, NumericType SIGN = 1.0);
  void parseInequality(const std::string& comparisonOperator);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_LinearExpression_H