#ifndef BPMNOS_Model_ExpressionOperator_H
#define BPMNOS_Model_ExpressionOperator_H

#include <exprtk.hpp>

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Operator.h"
#include "model/parser/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator that uses an expression to determine that value
 * of a status attribute.
 **/
class ExpressionOperator : public Operator {
public:
  ExpressionOperator(XML::bpmnos::tOperator* operator_, const AttributeMap& attributeMap);
  std::unique_ptr<Expression> expression;

  void apply(Values& values) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpressionOperator_H