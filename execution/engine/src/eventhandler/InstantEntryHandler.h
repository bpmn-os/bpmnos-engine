#ifndef BPMNOS_Execution_InstantEntryHandler_H
#define BPMNOS_Execution_InstantEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity that is not a child of a sequential ad-hoc subprocess.
 */
class InstantEntryHandler : public EventHandler {
public:
  InstantEntryHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void subscribe(Engine* engine);
  void notice(EntryEvent* event);
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<Event> > parallelEntryEvents;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantEntryHandler_H

