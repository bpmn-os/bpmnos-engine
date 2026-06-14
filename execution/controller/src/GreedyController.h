#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller making layered decisions.
 *
 * It first clears the unambiguous decisions (`prioritizedDispatchers`) in priority order,
 * dispatching the first feasible one, and only falls back to best-of-best over the
 * contested decisions (`competingDispatchers`).
 */
class GreedyController : public Controller {
public:
  GreedyController(Evaluator* evaluator);
  void connect(Mediator* mediator);
  std::vector< std::unique_ptr<EventDispatcher> > prioritizedDispatchers;   ///< Dispatched first-feasible in priority order.
  std::vector< std::unique_ptr<EventDispatcher> > competingDispatchers; ///< Dispatched by best-of-best reward.
protected:
  Evaluator* evaluator;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

