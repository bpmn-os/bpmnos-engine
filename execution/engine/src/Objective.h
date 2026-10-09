#ifndef BPMNOS_Execution_Objective_H
#define BPMNOS_Execution_Objective_H

#include <nlohmann/json.hpp>
#include "Observable.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Notification that the objective of a run has changed, carrying its new value and the change, the
 * previous value being the new value minus the change.
 *
 * The objective is held by the system state. It is the sum of the weighted values of the global attributes,
 * of the data attributes of every scope created so far, and of the final status attributes of every scope
 * that has ended, each multiplied by the weight of the attribute.
 */
struct Objective : Observable {
  constexpr Type getObservableType() const override { return Type::Objective; };
  Objective(BPMNOS::number value, BPMNOS::number change) : value(value), change(change) {}
  const BPMNOS::number value; ///< The new value of the objective
  const BPMNOS::number change; ///< The change of the objective, which is not zero

  /// @brief Returns the new value and the change as {"value": v, "change": c}.
  nlohmann::ordered_json jsonify() const { return { {"value", (double)value}, {"change", (double)change} }; }
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Objective_H
