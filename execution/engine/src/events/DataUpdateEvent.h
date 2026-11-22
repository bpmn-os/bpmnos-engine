#ifndef BPMNOS_Execution_DataUpdateEvent_H
#define BPMNOS_Execution_DataUpdateEvent_H

#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/Object.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Execution {

class SystemState;
class StateMachine;

/**
 * @brief Represents the event of the environment of a run updating data or global attributes.
 *
 * The event names the data it changes by the scope declaring it and the identifier of the state machine holding it,
 * which together name exactly one live state machine: the state machine of a process instance, of an instance of a
 * multi-instance activity and of an instance of a non-interrupting event subprocess has an identifier of its own,
 * and any other state machine shares the identifier of its parent, of which only one is alive at a time for a given
 * scope. Global attributes are named without a scope. The event gives new values of scalar attributes, a value
 * being undefined if it is not given, and new objects of object attributes.
 *
 * Processing the event writes the values as an operator writes them, adds the changes of weighted attributes to the
 * objective, and announces a @ref DataUpdate naming the process instance, or the instance 0 for global attributes,
 * so that restrictions, conditional events, deferrals and cached evaluations reading an attribute written are
 * reconsidered as after any other change of data. The event takes effect at the time the run has reached; it changes
 * neither the time, which only a @ref ClockTickEvent changes, nor the timestamp of any token.
 */
struct DataUpdateEvent : Event {
  using Values = std::vector< std::pair<const BPMNOS::Model::Attribute*, BPMNOS::Value> >;
  using Objects = std::vector< std::pair<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>> >;

  /**
   * @brief Constructs the update of global attributes.
   * @throws std::invalid_argument if an attribute is not a global attribute that may be updated
   */
  DataUpdateEvent(const SystemState* systemState, Values values, Objects objects = {});

  /**
   * @brief Constructs the update of the data the given scope declares, held by the state machine of the given
   * identifier.
   * @throws std::invalid_argument if an attribute is not a data attribute of the scope that may be updated
   */
  DataUpdateEvent(const SystemState* systemState, const BPMN::Scope* scope, BPMNOS::number instance, Values values, Objects objects = {});

  void processBy(Engine* engine) const override;

  /// Stale once the state machine holding the data no longer exists.
  bool expired() const override;

  nlohmann::ordered_json jsonify() const override;

  /// @brief Returns the state machine holding the data, the global state machine for global attributes, or nullptr if
  /// it no longer exists.
  StateMachine* owner() const;

  const SystemState* systemState;
  const BPMN::Scope* scope; ///< The scope declaring the data, nullptr for global attributes
  BPMNOS::number instance; ///< The identifier of the state machine holding the data, 0 for global attributes
  Values values;
  Objects objects;

private:
  /// @brief Refuses an attribute that the event cannot update.
  void validate(const BPMNOS::Model::Attribute* attribute, bool isObject) const;
  /// @brief Returns the state machine of the scope and identifier of the event at or below the given one, if any.
  StateMachine* find(StateMachine* stateMachine) const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdateEvent_H
