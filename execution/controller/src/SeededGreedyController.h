#ifndef BPMNOS_Execution_SeededGreedyController_H
#define BPMNOS_Execution_SeededGreedyController_H

#include <bpmn++.h>
#include "CPController.h"
#include "CPSeed.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching the best evaluated decisions
 */
class SeededGreedyController : public CPController {
public:
  SeededGreedyController(const BPMNOS::Model::Scenario* scenario, Evaluator* evaluator);
  void connect(Mediator* mediator);
  bool setSeed(const std::list<size_t>& seed);
protected:
  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
  Evaluator* evaluator;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
  CPSeed _seed;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SeededGreedyController_H

