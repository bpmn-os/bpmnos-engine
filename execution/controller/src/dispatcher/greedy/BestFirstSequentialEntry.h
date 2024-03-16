#ifndef BPMNOS_Execution_BestFirstSequentialEntry_H
#define BPMNOS_Execution_BestFirstSequentialEntry_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/events/EntryDecision.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class BestFirstSequentialEntry : public EventDispatcher, public Observer {
public:
  BestFirstSequentialEntry();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;

  void entryRequest(const DecisionRequest* request);
  void sequentialPerformerUpdate(const SequentialPerformerUpdate* update);
private:
  std::map< std::weak_ptr<const Token>, auto_set< BPMNOS::number, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >, std::owner_less<> > tokensAtIdlePerformers;
  std::map< std::weak_ptr<const Token>, auto_set< BPMNOS::number, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >, std::owner_less<> > tokensAtBusyPerformers;
  BPMNOS::number cost(const Token* token);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstSequentialEntry_H

