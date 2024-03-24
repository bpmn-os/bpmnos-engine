#ifndef BPMNOS_Model_AssignOperator_H
#define BPMNOS_Model_AssignOperator_H

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

namespace BPMNOS::Model {


/**
 * @brief Class representing an operator that assigns a value to a status attribute.
 **/
class AssignOperator : public Operator {
public:
  AssignOperator(XML::bpmnos::tOperator* operator_, AttributeMap& statusAttributes);
  Parameter* parameter;

/**
 * @brief Assigns a a value to a status attribute.
 *
 * This function Assigns a status attribute based on the provided parameter configuration.
 * The function performs the following steps:
 *
 * - If the parameter specifies an attribute and the status contains a value for
 *   that attribute, the value from the respective attribute is used to update
 *   the target attribute in the status.
 * - If the parameter doesn't specify an attribute or the status doesn't contain
 *   a value for that attribute, but a parameter value is available, the parameter
 *   value is used to update the target attribute in the status.
 * - If neither a parameter attribute with value nor a parameter value is available,
 *   the target attribute in the status is Assign to undefined (std::nullopt).
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AssignOperator_H
