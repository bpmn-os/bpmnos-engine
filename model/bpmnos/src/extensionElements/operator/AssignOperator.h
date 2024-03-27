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
  AssignOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);
  Parameter* parameter;

/**
 * @brief Assigns a a value to a status or data attribute.
 *
 * This function sets a status or data attribute based on the provided parameter configuration.
 * The function performs the following steps:
 *
 * - If the parameter specifies an attribute and the status or data contains a value for
 *   that attribute, the value from the respective attribute is used to update
 *   the target attribute.
 * - If the parameter doesn't specify an attribute or the status or data doesn't contain
 *   a value for that attribute, but a parameter value is available, the parameter
 *   value is used to update the target attribute.
 * - If neither a parameter attribute with value nor a parameter value is available,
 *   the target attribute is set to undefined (std::nullopt).
 */
  template <typename DataType>
  void _apply(BPMNOS::Values& status, DataType& data) const;

  void apply(BPMNOS::Values& status, BPMNOS::Values& data) const override { return _apply(status,data); };
  void apply(BPMNOS::Values& status, BPMNOS::Globals& data) const override { return _apply(status,data); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AssignOperator_H
