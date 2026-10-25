#ifndef BPMNOS_Model_EscalationDefinition_H
#define BPMNOS_Model_EscalationDefinition_H

#include "model/utility/src/Schema.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/xml/bpmnos/tEscalation.h"

namespace BPMNOS::Model {

/**
 * @brief The definition an escalation states: its name and the type of its content, which has no header.
 */
class EscalationDefinition {
public:
  EscalationDefinition(XML::bpmnos::tEscalation* escalation);
  XML::bpmnos::tEscalation* element;
  BPMNOS::number name; ///< Escalation name
  Schema schema = ObjectType{}; ///< The type of the content, an object, without fields if the escalation states no type
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_EscalationDefinition_H
