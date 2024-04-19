#ifndef BPMNOS_Model_ExpressionOperator_H
#define BPMNOS_Model_ExpressionOperator_H

#include <exprtk.hpp>

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/Operator.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator that uses an expression on status and data attributes.
 **/
class ExpressionOperator : public Operator {
public:
  ExpressionOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);
  std::unique_ptr<Expression> expression;

  template <typename DataType>
  void _apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const;

  void apply(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const override { return _apply(status,data,globals); };
  void apply(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const override { return _apply(status,data,globals); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpressionOperator_H
