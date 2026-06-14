#ifndef BPMNOS_Execution_GreedyEntry_H
#define BPMNOS_Execution_GreedyEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the first feasible entry event for a token awaiting the entry at an activity or
 * the best token awaiting the entry at an activity performed by a sequential performer.
 *
 * If `config.sequential` is false, entries of activities that are children of a
 * SequentialAdHocSubProcess are skipped (left to be dispatched by another dispatcher)
 * rather than competed for their performer.
 */
class GreedyEntry : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  struct Config {
    bool sequential = true; ///< If false, skip entries of SequentialAdHocSubProcess children.
  };
  static Config default_config() { return {}; }  // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051

  GreedyEntry(Evaluator* evaluator, Config config = default_config());
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
protected:
  Config config;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > unevaluatedSequentialEntries;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyEntry_H

