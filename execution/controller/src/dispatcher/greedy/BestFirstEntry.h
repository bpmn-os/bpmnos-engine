#ifndef BPMNOS_Execution_BestFirstEntry_H
#define BPMNOS_Execution_BestFirstEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the best entry event for a token awaiting the entry at an activity.
 */
class BestFirstEntry : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  struct Config {
    bool onlySequential = false; ///< If true, only entries of SequentialAdHocSubProcess children are handled.
  };
  static Config default_config() { return {}; } // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051

  BestFirstEntry(Evaluator* evaluator, Config config = default_config());
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
protected:
  Config config;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstEntry_H

