#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include "Controller.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best evaluated decisions
 */
class GreedyController : public Controller {
public:
  GreedyController();
  void connect(Mediator* mediator);
  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

