#include "DecisionTask.h"
#include "extensionElements/ExtensionElements.h"
#include <cassert>

using namespace BPMNOS::Model;

DecisionTask::DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent)
  : BPMN::Node(task)
  , BPMN::FlowNode(task,parent)
  , BPMN::Task(task,parent)
{
}

template <typename DataType>
std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  assert(extensionElements->represents<ExtensionElements>());
  auto extensionElements = this->extensionElements->as<ExtensionElements>();
  assert(!extensionElements->choices.empty());

  BPMNOS::Values statusCopy = status;
  BPMNOS::Values dataCopy = data;
  BPMNOS::Values globalsCopy = globals;
  std::vector<std::vector<BPMNOS::number>> alternativeChoices;
  std::vector<BPMNOS::number> tmp(extensionElements->choices.size());
  determineAlternatives(alternativeChoices, extensionElements, statusCopy, dataCopy, globalsCopy, tmp, 0);

  return alternativeChoices;
}

template std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;

template std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


void DecisionTask::determineAlternatives(
  std::vector<std::vector<BPMNOS::number>>& alternatives,
  const ExtensionElements* extensionElements,
  BPMNOS::Values& status,
  BPMNOS::Values& data,
  BPMNOS::Values& globals,
  std::vector<number>& choices,
  size_t index
) {
  assert(index < choices.size());
  auto& choice = extensionElements->choices[index];

  auto choose = [&](number value) -> void {
    choices[index] = value;
    choice->attributeRegistry.setValue(choice->attribute, status, data, globals, value);
    if ( index + 1 == choices.size() ) {
      alternatives.push_back(choices);
    }
    else {
      determineAlternatives(alternatives, extensionElements, status, data, globals, choices, index + 1);
    }
  };

  if ( !choice->enumeration.empty() || choice->multipleOf ) {
    // iterate through all given alternatives
    for (auto value : choice->getEnumeration(status, data, globals) ) {
      choose(value);
    }
  }
  else if ( choice->lowerBound.has_value() && choice->upperBound.has_value() ) {
    auto [min, max] = choice->getBounds(status, data, globals);
    if ( min == max ) {
      choose(min);
    }
    else if ( min < max ) {
      choose(min);
      choose(max);
    }
  }
}
