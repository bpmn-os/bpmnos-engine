#ifndef BPMNOS_Execution_RandomChoiceHandler_H
#define BPMNOS_Execution_RandomChoiceHandler_H

#include <bpmn++.h>
#include "model/utility/src/RandomDistributionFactory.h"
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a random choice event for a token at a decision task.
 *
 * The RandomChoiceHandler creates a random choice event within the limits implied by the 
 * exit restrictions at the @ref BPMNOS::Model::DecisionTask. These limits are deduced
 * from the attribute type of the decision and stricter decisions may be provided
 * through restrictions of type @ref BPMNOS::Model::LinearExpression. If these stricter
 * limits imply an empty domain, an error event is created. It is assumed that the existence
 * of a feasible choice does not depend on the timestamp for any moment after entry
 * of the decision task and until the decision is made.
 */
class RandomChoiceHandler : public EventHandler {
public:
  RandomChoiceHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  BPMNOS::RandomGenerator randomGenerator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_RandomChoiceHandler_H

