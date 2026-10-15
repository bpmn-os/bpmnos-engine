#ifndef BPMNOS_Model_SignalDefinition_H
#define BPMNOS_Model_SignalDefinition_H

#include "Schema.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"

namespace BPMNOS::Model {

/**
 * @brief The definition a signal states: its name and the type of its content, which has no header.
 */
class SignalDefinition {
public:
  SignalDefinition(XML::bpmnos::tSignal* signal);
  XML::bpmnos::tSignal* element;
  BPMNOS::number name; ///< Signal name
  Schema schema; ///< The type of the content, an object, without fields if the signal states no type
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SignalDefinition_H
