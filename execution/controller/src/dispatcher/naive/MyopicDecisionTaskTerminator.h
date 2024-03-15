#ifndef BPMNOS_Execution_MyopicDecisionTaskTerminator_H
#define BPMNOS_Execution_MyopicDecisionTaskTerminator_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an error event for a token at a decision task if no choice has been made.
 *
 * The MyopicDecisionTaskTerminator terminates a @ref BPMNOS:Model::DecisionTask with an error if
 * no choice has been made previously. It assumes that a handler making potential choices has been called before.
 * The handler is myopic and does not consider that an increase in the timestamp may affect possible choices.
 */
class MyopicDecisionTaskTerminator : public EventDispatcher {
public:
  MyopicDecisionTaskTerminator();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MyopicDecisionTaskTerminator_H

