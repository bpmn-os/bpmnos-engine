#ifndef BPMNOS_Execution_FirstComeFirstServedSequentialEntry_H
#define BPMNOS_Execution_FirstComeFirstServedSequentialEntry_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/events/EntryDecision.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class FirstComeFirstServedSequentialEntry : public EventDispatcher, public Observer {
public:
  FirstComeFirstServedSequentialEntry();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;

  void entryRequest(const EntryDecision* event);
  void sequentialPerformerUpdate(const SequentialPerformerUpdate* update);
private:
  std::map< std::weak_ptr<const Token>, auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> >, std::owner_less<> > tokensAtIdlePerformers;
  std::map< std::weak_ptr<const Token>, auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> >, std::owner_less<> > tokensAtBusyPerformers;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstComeFirstServedSequentialEntry_H

