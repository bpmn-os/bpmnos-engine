#ifndef BPMNOS_Model_UnassignOperator_H
#define BPMNOS_Model_UnassignOperator_H

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator that unassigns the value from an attribute.
 **/
class UnassignOperator : public Operator {
public:
  UnassignOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);

/**
 * @brief Sets an attribute value to undefined.
 */
  template <typename DataType>
  void _apply(BPMNOS::Values& status, DataType& data) const;

  void apply(BPMNOS::Values& status, BPMNOS::Values& data) const override { return _apply(status,data); };
  void apply(BPMNOS::Values& status, BPMNOS::SharedValues& data) const override { return _apply(status,data); };

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_UnassignOperator_H
