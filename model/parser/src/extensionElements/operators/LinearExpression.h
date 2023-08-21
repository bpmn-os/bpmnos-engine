#ifndef BPMNOS_LinearExpression_H
#define BPMNOS_LinearExpression_H

#include <vector>
#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "Expression.h"

namespace BPMNOS {

class LinearExpression : public Expression {
public:
  LinearExpression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  Parameter* parameter;
 
  using Term = std::tuple<number,Attribute*>;
  std::vector<Term> terms;

/**
 * @brief Applies the linear expression to update a status value.
 *
 * This updates the attribute value based on a linear expression composed of terms.
 * Linear expressions must follow these rules:
 * - Terms are separated by operators "+" or "-".
 * - Each term may consist of: 
 *   - a coefficient represented by a number, Keyword::True, or Keyword::False,
 *   - an operator "*" or "/"
 *   - and a variable represented by an attribute name.
 * - The denominator of a division must be a coefficient.
 * - Alternatively a term may consist of only a coeffcient or only a variable. 
 * - If any of the variables in the expression is undefined, the result for the
 *   attribute is set to `std::nullopt`.
 * - Keyword::True and Keyword::False are replaced by 1 and 0, respectively.
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS

#endif // BPMNOS_LinearExpression_H
