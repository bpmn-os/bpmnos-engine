#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best evaluated decisions
 */
class GreedyController : public Controller {
public:
  struct Config {
    bool bestFirstEntry = true;
    bool bestFirstExit = true;
  };
  static Config default_config() { return {}; } // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  GreedyController(Evaluator* evaluator, Config config = default_config());
  void connect(Mediator* mediator);
  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
protected:
  Evaluator* evaluator;
  Config config;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

