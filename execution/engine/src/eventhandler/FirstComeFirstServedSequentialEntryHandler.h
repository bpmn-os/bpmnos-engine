#ifndef BPMNOS_Execution_FirstComeFirstServedSequentialEntryHandler_H
#define BPMNOS_Execution_FirstComeFirstServedSequentialEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class FirstComeFirstServedSequentialEntryHandler : public EventHandler, public Observer {
public:
  FirstComeFirstServedSequentialEntryHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;

  void noticeEntryEvent(const EntryEvent* event);
  void noticeSequentialPerformerUpdate(const SequentialPerformerUpdate* update);
private:
  std::map< std::weak_ptr<const Token>, auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> >, std::owner_less<> > tokensAtIdlePerformers;
  std::map< std::weak_ptr<const Token>, auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> >, std::owner_less<> > tokensAtBusyPerformers;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstComeFirstServedSequentialEntryHandler_H

