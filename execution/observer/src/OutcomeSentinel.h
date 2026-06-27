#ifndef BPMNOS_Execution_OutcomeSentinel_H
#define BPMNOS_Execution_OutcomeSentinel_H

#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Observer.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

enum class Outcome { COMPLETED, FAILED, TERMINATED, INCOMPLETE };
static inline std::string outcome[] = { "COMPLETED", "FAILED", "TERMINATED", "INCOMPLETE" };

/**
 * @brief Class observing the engine to be able to provide an outcome.
 */
class OutcomeSentinel : public Observer {
public:
  void subscribe(Engine* engine);
  Outcome getOutcome() const;
private:
  void notice(const Observable* observable) override;
  unsigned int runningInstances;
  std::optional<Outcome> firstObservation;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_OutcomeSentinel_H

