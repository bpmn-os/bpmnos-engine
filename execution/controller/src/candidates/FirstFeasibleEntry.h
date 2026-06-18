#ifndef BPMNOS_Execution_FirstFeasibleEntry_H
#define BPMNOS_Execution_FirstFeasibleEntry_H

#include <bpmn++.h>
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Entry decision candidates dispatched greedily as the first feasible entry.
 *
 * Collects entry requests via notice; evaluateCandidates creates and evaluates the pending entry
 * decisions only until the first feasible one, which is then the front of the reward-ordered set.
 * If config.sequential is false (default), entries of SequentialAdHocSubProcess children are skipped
 * (left to a competing source) rather than considered here.
 */
class FirstFeasibleEntry : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  struct Config {
    bool sequential = false; ///< If false, skip entries of SequentialAdHocSubProcess children.
  };
  static Config default_config() { return {}; }  // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051

  FirstFeasibleEntry(Evaluator* evaluator, Config config = default_config());
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
protected:
  void evaluateCandidates() override;
  Evaluator* evaluator;
  Config config;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstFeasibleEntry_H
