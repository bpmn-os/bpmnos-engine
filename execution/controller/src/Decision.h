#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

class Evaluator;

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : virtual public Event {
public:
  Decision(Evaluator* evaluator);

  virtual std::optional<double> evaluate() = 0; ///< Evaluates the reward for the decision. Returns null if decision is infeasible.
  std::optional<double> reward;  ///< Latest evaluated reward or null if decision has not been evaluated or reward is no longer valid
  
  bool timeDependent;
  std::set<const BPMNOS::Model::Attribute*> dataDependencies;
protected:
  void determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies);
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
