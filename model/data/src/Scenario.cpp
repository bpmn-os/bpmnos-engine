#include "Scenario.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

using namespace BPMNOS::Model;

Scenario::Scenario(const Model* model, const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap)
  : model(model)
  , globals(evaluateGlobals(globalValueMap))
{
}

Scenario::Scenario(const Scenario* original)
  : model(original->model)
  , globals(original->globals)
  , taskCompletionStatus(original->taskCompletionStatus)
  , activityArrivalStatus(original->activityArrivalStatus)
{
}

BPMNOS::Values Scenario::evaluateGlobals(const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap) {
  Values result(model->attributes.size());

  // Set CSV-provided globals
  for (auto& [attribute, value] : globalValueMap) {
    result[attribute->index] = value;
  }

  // Evaluate model-defined global expressions
  for (auto& attribute : model->attributes) {
    if (attribute->expression && !result[attribute->index].has_value()) {
      auto value = attribute->expression->execute(Values{}, Values{}, result);
      if (!value.has_value()) {
        throw std::runtime_error("Scenario: failed to evaluate global attribute '" + attribute->id + "'");
      }
      result[attribute->index] = value.value();
    }
  }

  return result;
}

std::unique_ptr<Scenario> Scenario::clone() const {
  throw std::logic_error("Scenario::clone() is only supported for deterministic scenarios");
}

void Scenario::noticeCompletionPending([[maybe_unused]] BPMNOS::number rootId, const BPMN::Node* task, const Values& status, const SharedValues& data, [[maybe_unused]] const Values& globals) const {
  // key by the full instance id (carries ^...#k for event-subprocess/multi-instance executions)
  auto instanceId = (size_t)data[ExtensionElements::Index::Instance].get().value();
  taskCompletionStatus[{instanceId, task}] = status;
}

BPMNOS::Values Scenario::getTaskCompletionStatus(BPMNOS::number rootId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const {
  noticeCompletionPending(rootId,task,status,data,globals);
  auto instanceId = (size_t)data[ExtensionElements::Index::Instance].get().value();
  Values& completionStatus = taskCompletionStatus[{instanceId, task}];
  if ( status[ExtensionElements::Index::Timestamp] != completionStatus[ExtensionElements::Index::Timestamp] ) {
    throw std::runtime_error("Scenario: illegal change of completion time for node '" + task->id + "'");
  }
  return completionStatus;
}

std::optional<BPMNOS::Values> Scenario::getTaskCompletionStatus(
    BPMNOS::number instanceId,
    const BPMN::Node* task,
    BPMNOS::number currentTime) const {
  auto key = std::make_pair((size_t)instanceId, task);
  if (!taskCompletionStatus.contains(key)) {
    return std::nullopt;
  }
  auto& status = taskCompletionStatus.at(key);
  if (currentTime >= status[ExtensionElements::Index::Timestamp]) {
    return status;
  }
  return std::nullopt;
}
