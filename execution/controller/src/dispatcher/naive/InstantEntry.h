#ifndef BPMNOS_Execution_InstantEntry_H
#define BPMNOS_Execution_InstantEntry_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity that is not a child of a sequential ad-hoc subprocess.
 */
class InstantEntry : public EventDispatcher, public Observer {
public:
  InstantEntry();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > parallelEntryRequests;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantEntry_H

