#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the first feasible decision in priority order.
 *
 * Holds a priority-ordered list of dispatchers and dispatches the first feasible decision they yield.
 * Contested entries and message deliveries (which have no precedence over each other) are merged into a
 * single reward-ordered dispatcher, so their best feasible decision competes within the same list rather
 * than in a separate best-of-best stage.
 */
class GreedyController : public Controller {
public:
  struct Config {
    bool bisection = false; ///< If true, use FirstBisectionalChoice, otherwise use FirstEnumeratedChoice.
  };
  static Config default_config() { return {}; }  // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  GreedyController(Evaluator* evaluator, Config config = default_config());
  void connect(Mediator* mediator) override;
  std::vector< std::unique_ptr<EventDispatcher> > dispatchers;   ///< Dispatched first-feasible in priority order.
protected:
  Evaluator* evaluator;
  Config config;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

