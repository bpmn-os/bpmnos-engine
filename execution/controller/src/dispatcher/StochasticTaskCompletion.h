#ifndef BPMNOS_Execution_StochasticTaskCompletion_H
#define BPMNOS_Execution_StochasticTaskCompletion_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Token.h"
#include "execution/utility/src/auto_set.h"

namespace BPMNOS::Execution {

class Engine;

/**
 * @brief Class creating a completion event with stochastic status for a token awaiting task completion.
 *
 * Observes tokens entering BUSY state at tasks. Captures status and stores it for later dispatch.
 * When the completion time is reached, dispatches CompletionEvent with the stored status.
 */
class StochasticTaskCompletion : public EventDispatcher, public Observer {
public:
  StochasticTaskCompletion();

  void subscribe(Engine* engine);

  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;

  void notice(const Observable* observable) override;

private:
  /// Stored status updates keyed by completion time and token pointer
  auto_set<BPMNOS::number, std::weak_ptr<Token>, std::optional<Values> > pendingCompletions;

  /// Creates the (stochastic) status update for a token
  BPMNOS::Values createStatusUpdate(const Token* token);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StochasticTaskCompletion_H
