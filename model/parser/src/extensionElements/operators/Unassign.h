#ifndef BPMNOS_Model_Unassign_H
#define BPMNOS_Model_Unassign_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Operator.h"

namespace BPMNOS::Model {

class Unassign : public Operator {
public:
  Unassign(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);

/**
 * @brief Sets a status attribute as undefined.
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Unassign_H
