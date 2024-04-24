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

  virtual std::optional<double> evaluate() = 0;
  std::optional<double> evaluation;  ///< Latest evaluation or null if decision has not been evaluated or evaluation is no longer valid
  
  bool timeDependent;
  std::set<const BPMNOS::Model::Attribute*> dataDependencies;
  
  virtual nlohmann::ordered_json jsonify() const = 0;

protected:
  void determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies);
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
