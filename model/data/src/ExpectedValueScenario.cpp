#include "ExpectedValueScenario.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Model;

ExpectedValueScenario::ExpectedValueScenario(
  const Model* model,
  BPMNOS::number earliestInstantiationTime,
  BPMNOS::number latestInstantiationTime,
  const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap
)
  : StaticScenario(model, earliestInstantiationTime, latestInstantiationTime, globalValueMap)
{
}

void ExpectedValueScenario::addCompletionExpression(
  const BPMNOS::number instanceId,
  const BPMN::Node* task,
  ExpectedCompletionExpression&& expr
) {
  completionExpressions[(size_t)instanceId][task].push_back(std::move(expr));
}

void ExpectedValueScenario::setTaskCompletionStatus(
  const BPMNOS::number instanceId,
  const BPMN::Node* task,
  BPMNOS::Values status
) const {
  size_t id = (size_t)instanceId;

  // Check if we have completion expressions for this (instance, task)
  if (completionExpressions.contains(id) &&
      completionExpressions.at(id).contains(task) &&
      !completionExpressions.at(id).at(task).empty()) {

    auto& taskCompletionExpressions = completionExpressions.at(id).at(task);

    // Get the task's extension elements
    auto extensionElements = task->extensionElements->as<const ExtensionElements>();

    // Build data values from instance
    auto& instance = instances.at(id);
    Values data;
    for (auto& attribute : extensionElements->data) {
      if (instance.values.contains(attribute.get())) {
        data.push_back(instance.values.at(attribute.get()));
      }
      else {
        data.push_back(std::nullopt);
      }
    }

    // Evaluate completion expressions and update status
    // (Expected value functions are already registered on the model's limexHandle)
    for (auto& completionExpression : taskCompletionExpressions) {
      auto value = completionExpression.expression->execute(status, data, globals);
      if (value.has_value() && completionExpression.attribute) {
        // Find attribute index in status
        size_t index = 0;
        for (auto& attribute : extensionElements->attributes) {
          if (attribute.get() == completionExpression.attribute) {
            status[index] = value;
            break;
          }
          ++index;
        }
      }
    }
  }

  // Store the (possibly modified) status
  taskCompletionStatus[{id, task}] = std::move(status);
}
