#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

class Evaluator;

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : virtual public Event {
public:
  Decision();

  virtual std::optional<double> evaluate(Evaluator* evaluator) = 0;
  std::optional<double> evaluation;  ///< Latest evaluation or null if decision has not been evaluated or evaluation is no longer valid
  
  bool timeDependent;
  std::set<const BPMNOS::Model::Attribute*> dataDependencies;
  void determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
