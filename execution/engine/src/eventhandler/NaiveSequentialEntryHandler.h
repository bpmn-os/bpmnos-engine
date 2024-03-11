#ifndef BPMNOS_Execution_NaiveSequentialEntryHandler_H
#define BPMNOS_Execution_NaiveSequentialEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"
#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class NaiveSequentialEntryHandler : public EventHandler, public Observer {
public:
  NaiveSequentialEntryHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> > sequentialEntryEvents;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_NaiveSequentialEntryHandler_H

