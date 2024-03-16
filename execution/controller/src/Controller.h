#ifndef BPMNOS_Execution_Controller_H
#define BPMNOS_Execution_Controller_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

/**
 * @brief Base class for an execution controller which dispatches events
 * to the engine and listens to notification from the engine. 
 */
class Controller : public Observer, public EventDispatcher, public Mediator {
public:
  Controller();
  virtual ~Controller() = default;
  virtual void connect(Mediator* mediator);
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Controller_H

