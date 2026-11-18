#ifndef BPMNOS_Execution_GreedyController_H
#define BPMNOS_Execution_GreedyController_H

#include <bpmn++.h>
#include <optional>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best feasible decision of all.
 *
 * Holds one dispatcher for each kind of decision, the choices, the entries of children of sequential ad-hoc
 * subprocesses, and the message deliveries that are not directly addressed, the others being taken by the engine.
 * Each dispatcher yields the best feasible decision of its candidates, and the controller dispatches the one of the
 * highest reward among them, which is the best feasible decision of all, a tie going to the dispatcher listed first.
 * No kind of decision takes precedence over another.
 */
class GreedyController : public Controller {
public:
  struct Config {
    std::optional<size_t> bisect = std::nullopt; ///< The least number of values of a choice searched by bisection, none if only the choices of a decimal attribute without a step are (see Choices).
  };
  static Config default_config() { return {}; }  // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  /// The evaluator is handed to the candidates the dispatchers own, which keep it; the controller does not.
  GreedyController(std::shared_ptr<Evaluator> evaluator, Config config = default_config());
  void connect(Mediator* mediator) override;
  std::vector< std::unique_ptr<EventDispatcher> > dispatchers;   ///< The dispatchers whose best decisions are compared, a tie going to the one listed first.
protected:
  Config config;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyController_H

