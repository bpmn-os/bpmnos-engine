#include "Scenario.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

using namespace BPMNOS::Model;

Scenario::Scenario(const Model* model, const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap)
  : model(model)
  , globals(evaluateGlobals(globalValueMap))
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
