#ifndef BPMNOS_Unset_H
#define BPMNOS_Unset_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Operator.h"

namespace BPMNOS {

class Operator;

class Unset : public Operator {
public:
  Unset(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);

/**
 * @brief Sets a status attribute as undefined.
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS

#endif // BPMNOS_Unset_H
