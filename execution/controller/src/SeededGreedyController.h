#ifndef BPMNOS_Execution_SeededGreedyController_H
#define BPMNOS_Execution_SeededGreedyController_H

#include <bpmn++.h>
#include "SeededController.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"
#include "dispatcher/greedy/BestLimitedChoice.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best evaluated decisions
 */
class SeededGreedyController : public SeededController {
public:
  SeededGreedyController(const BPMNOS::Execution::FlattenedGraph* flattenedGraph, Evaluator* evaluator);
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
protected:
  auto_list< std::weak_ptr<const Message> > messages;
  std::unique_ptr<BestLimitedChoice> choiceDispatcher;
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SeededGreedyController_H

