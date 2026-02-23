#ifndef BPMNOS_Execution_CPController_H
#define BPMNOS_Execution_CPController_H

#include <bpmn++.h>
#include "SeededController.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Message.h"
#include "execution/model/src/CPModel.h"
#include <cp/solver.h>

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best decisions obtained from a CP solution
 */
class CPController : public SeededController {
public:
  CPController(const BPMNOS::Execution::FlattenedGraph* flattenedGraph);

  void setSolution(CP::Solution solution);

  void notice(const Observable* observable) override;
  std::shared_ptr<Event> createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;
  std::shared_ptr<Event> createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) override;

  CPModel constraintProgramm;

protected:
  std::unique_ptr<CP::Solution> solution;
  auto_list< std::weak_ptr<const Message>, const Vertex* > messages;

  bool isVisited(const Vertex* vertex) const;
  BPMNOS::number getTimestamp(const Vertex* vertex) const;

private:
  void initializeFromSolution();
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPController_H

