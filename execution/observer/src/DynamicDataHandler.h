#ifndef BPMNOS_Execution_DynamicDataHandler_H
#define BPMNOS_Execution_DynamicDataHandler_H

#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

/**
 * @brief Observer that handles deferred data disclosure.
 *
 * This observer monitors clock tick events and triggers evaluation
 * of deferred initialization expressions when their disclosure time
 * is reached.
 */
class DynamicDataHandler : public Observer {
public:
  DynamicDataHandler() = default;
  ~DynamicDataHandler() = default;

  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DynamicDataHandler_H
