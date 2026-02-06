#ifndef BPMNOS_Execution_RandomChoice_H
#define BPMNOS_Execution_RandomChoice_H

#include <bpmn++.h>
#include "model/utility/src/RandomDistributionFactory.h"
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a random choice event for a token at a decision task.
 *
 * The RandomChoice dispatcher creates a the random choice considering the
 * boundaries for a numeric decision or any enumerated value for a string decision.
 * It is assumed that the existence of a feasible choice does not depend on the timestamp
 * for any moment after entry of the decision task and until the decision is made.
 */
class RandomChoice : public EventDispatcher {
public:
  RandomChoice();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  BPMNOS::RandomGenerator randomGenerator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_RandomChoice_H

