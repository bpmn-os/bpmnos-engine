#ifndef BPMNOS_Execution_Objective_H
#define BPMNOS_Execution_Objective_H

#include "Observable.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Notification that the objective of a run has changed, carrying its new value.
 *
 * The objective is held by the system state. It is the sum of the weighted values of the global attributes,
 * of the data attributes of every scope created so far, and of the final status attributes of every scope
 * that has ended, each multiplied by the weight of the attribute.
 */
struct Objective : Observable {
  constexpr Type getObservableType() const override { return Type::Objective; };
  Objective(BPMNOS::number value) : value(value) {}
  const BPMNOS::number value;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Objective_H
