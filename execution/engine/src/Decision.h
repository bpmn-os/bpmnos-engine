#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/Observable.h"
#include <functional>

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : public Event {
public:
  Decision(const Token* token, std::function<std::optional<double>(Decision* decision)> evaluator = &Decision::nullEvaluator);

  constexpr static std::optional<double> nullEvaluator([[maybe_unused]] Decision* decision) { return std::nullopt; }
  constexpr static std::optional<double> zeroEvaluator([[maybe_unused]] Decision* decision) { return 0; }

  std::optional<double> evaluate();
  std::optional<double> evaluation;
private:
  std::function< std::optional<double>(Decision* decision) > evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
