#ifndef BPMNOS_Execution_Controller_H
#define BPMNOS_Execution_Controller_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"
#include "execution/engine/src/Engine.h"

namespace BPMNOS::Execution {

/**
 * @brief Base class for an execution controller which dispatches events
 * to the engine and listens to notification from the engine. 
 */
class Controller : public EventHandler {
public:
  Controller();
  virtual void connect(Engine* engine);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Controller_H

