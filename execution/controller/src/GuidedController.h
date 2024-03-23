#ifndef BPMNOS_Execution_GuidedController_H
#define BPMNOS_Execution_GuidedController_H

#include <bpmn++.h>
#include "Controller.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best evaluated decisions
 */
class GuidedController : public Controller {
public:
  GuidedController();
  void connect(Mediator* mediator);
  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
protected:
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GuidedController_H

