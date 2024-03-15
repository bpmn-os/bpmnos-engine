#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/Observable.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending decision
 */
struct Decision : Event, Observable {
  Decision(const Token* token);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
