#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include "Controller.h"
#include "execution/engine/src/Engine.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller which dispatches the best evaluated decisions
 */
class GreedyController : public Controller {
public:
  GreedyController();
  void connect(Engine* engine);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

