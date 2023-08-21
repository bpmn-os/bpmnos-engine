#ifndef BPMNOS_Set_H
#define BPMNOS_Set_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Operator.h"

namespace BPMNOS {


class Set : public Operator {
public:
  Set(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  Parameter* parameter;

/**
 * @brief Sets a status attribute by applying the operator.
 *
 * This function sets a status attribute based on the provided parameter configuration.
 * The function performs the following steps:
 *
 * - If the parameter specifies an attribute and the status contains a value for
 *   that attribute, the value from the respective attribute is used to update
 *   the target attribute in the status.
 * - If the parameter doesn't specify an attribute or the status doesn't contain
 *   a value for that attribute, but a parameter value is available, the parameter
 *   value is used to update the target attribute in the status.
 * - If neither a parameter attribute with value nor a parameter value is available,
 *   the target attribute in the status is set to undefined (std::nullopt).
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS

#endif // BPMNOS_Set_H
