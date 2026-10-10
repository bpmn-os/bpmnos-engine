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
std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives(const BPMNOS::Status& status, const DataType& data) const {
  assert(extensionElements->represents<ExtensionElements>());
  auto extensionElements = this->extensionElements->as<ExtensionElements>();
  assert(!extensionElements->choices.empty());

  BPMNOS::Status statusCopy = status;
  BPMNOS::Data dataCopy = data;
  std::vector<std::vector<BPMNOS::number>> alternativeChoices;
  std::vector<BPMNOS::number> tmp(extensionElements->choices.size());
  determineAlternatives(alternativeChoices, extensionElements, statusCopy, dataCopy, tmp, 0);

  return alternativeChoices;
}

template std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;

template std::vector<std::vector<BPMNOS::number>> DecisionTask::enumerateAlternatives<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;


void DecisionTask::determineAlternatives(
  std::vector<std::vector<BPMNOS::number>>& alternatives,
  const ExtensionElements* extensionElements,
  BPMNOS::Status& status,
  BPMNOS::Data& data,
  std::vector<number>& choices,
  size_t index
) {
  assert(index < choices.size());
  auto& choice = extensionElements->choices[index];

  auto choose = [&](number value) -> void {
    choices[index] = value;
    choice->attributeRegistry.setValue(choice->attribute, status, data, value);
    if ( index + 1 == choices.size() ) {
      alternatives.push_back(choices);
    }
    else {
      determineAlternatives(alternatives, extensionElements, status, data, choices, index + 1);
    }
  };

  if ( !choice->enumeration.empty() || choice->multipleOf ) {
    // iterate through all given alternatives
    for (auto value : choice->getEnumeration(status, data) ) {
      choose(value);
    }
  }
  else if ( choice->lowerBound.has_value() && choice->upperBound.has_value() ) {
    auto [min, max] = choice->getBounds(status, data);
    if ( min == max ) {
      choose(min);
    }
    else if ( min < max ) {
      choose(min);
      choose(max);
    }
  }
}
