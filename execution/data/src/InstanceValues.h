#ifndef BPMNOS_Execution_InstanceValues_H
#define BPMNOS_Execution_InstanceValues_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <bpmn++.h>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/AttributeRegistry.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

class DataProvider;

/**
 * @brief The status and the owned data of a scope instance, built when the instance comes into being.
 *
 * The constructor lays out a slot for every own attribute of the scope, sets the timestamp to the time at
 * which the instance comes into being and gives the instance its identity, if it has one of its own. It then
 * walks over the own attributes of the scope in declaration order, the scalar status attributes, the scalar
 * data attributes and the objects, and gives each its initial value: the value of its initial expression if
 * the model initialises it, otherwise the value given, if any. An expression is evaluated over the status and
 * the data built so far, so that it reads everything inherited, the timestamp, the instance and everything
 * declared before it, and nothing declared after it.
 *
 * A data provider builds the values of a scope with a ready event when the event becomes due and applies its
 * ready values through @ref assign; the engine builds the values of the scopes it creates itself. The status
 * and the owned data are then moved into the ready event or into the scope created.
 */
class InstanceValues {
public:
  /**
   * @param dataProvider The data provider giving the default objects.
   * @param node The node whose scope instance comes into being.
   * @param time The time at which it comes into being.
   * @param inheritedStatus The status of the enclosing scopes the token holds, empty for a process.
   * @param inheritedData The data of the enclosing instances and the globals.
   * @param instance The identity of a scope instance having one of its own, a process instance or a
   *        non-interrupting event subprocess instance, std::nullopt for any other.
   * @param givenValues The values the instance data give.
   * @param givenObjects The objects the instance data give.
   */
  InstanceValues(
    const DataProvider& dataProvider,
    const BPMN::Node* node,
    BPMNOS::number time,
    BPMNOS::Status inheritedStatus,
    const BPMNOS::SharedData& inheritedData,
    std::optional<BPMNOS::number> instance,
    const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& givenValues,
    const std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>& givenObjects
  );
  /// The data chain refers to the members, so the values are neither copied nor moved.
  InstanceValues(const InstanceValues&) = delete;
  InstanceValues& operator=(const InstanceValues&) = delete;

  /**
   * @brief Evaluates an expression over the values built and writes its target, a status attribute or an owned
   * data attribute, a scalar or an object, element or field through its path. A scalar target keeps its value if
   * the expression gives none.
   */
  void assign(const BPMNOS::Model::Expression& expression);

  BPMNOS::Status status; ///< The inherited status followed by the own status attributes and objects of the scope
  BPMNOS::Data data; ///< The own data attributes and objects of the scope

private:
  const BPMNOS::Model::AttributeRegistry& attributeRegistry; ///< The attribute registry of the scope
  BPMNOS::Value identity; ///< The identity of a non-interrupting event subprocess instance the data chain refers to
  BPMNOS::SharedData chain; ///< The inherited data followed by the owned data
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstanceValues_H
