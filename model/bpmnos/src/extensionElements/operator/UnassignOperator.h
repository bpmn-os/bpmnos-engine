#ifndef BPMNOS_Model_UnassignOperator_H
#define BPMNOS_Model_UnassignOperator_H

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator that unassigns the value from a status attribute.
 **/
class UnassignOperator : public Operator {
public:
  UnassignOperator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);

/**
 * @brief Sets a status attribute as undefined.
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_UnassignOperator_H
