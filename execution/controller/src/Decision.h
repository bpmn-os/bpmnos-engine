#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include <functional>

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : virtual public Event {
public:
  Decision(std::function<std::optional<double>(const Event* event)> evaluator = &Decision::nullEvaluator);

  constexpr static std::optional<double> nullEvaluator([[maybe_unused]] const Event* event) { return std::nullopt; }
  constexpr static std::optional<double> zeroEvaluator([[maybe_unused]] const Event* event) { return 0; }

  virtual std::optional<double> evaluate() = 0;
  std::optional<double> evaluation;
protected:
  std::function< std::optional<double>(Event* event) > evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
