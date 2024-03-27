#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, std::function<std::optional<double>(const Event* event)> evaluator)
  : Event(token)
  , EntryEvent(token)
  , Decision(evaluator)
{
  evaluate();
}

std::optional<double> EntryDecision::evaluate() {
  evaluation = evaluator((EntryEvent*)this);
  return evaluation;
}

std::optional<double> EntryDecision::localEvaluator(const Event* event) {
  assert( event->token->ready() );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  Values data(event->token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);

  if ( 
    event->token->node->represents<BPMN::SubProcess>() ||
    event->token->node->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status,data);
  }

  if ( !extensionElements->feasibleEntry(status,data) ) {
    // entry would be infeasible
    return std::nullopt;
  }

  if ( event->token->node->represents<BPMN::Task>() && 
    !event->token->node->represents<BPMN::ReceiveTask>() &&
    !event->token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    // apply operators after checking entry restrictions and before applying guidance
    // receive tasks and decision tasks require further decision before operators are applied
    extensionElements->applyOperators(status,data);
    if ( !extensionElements->feasibleExit(status,data) ) {
      // entry would lead to infeasible exit
      return std::nullopt;
    }
  }

  // return evaluation of entry
  return evaluation - extensionElements->getObjective(status,data);
}

std::optional<double> EntryDecision::guidedEvaluator(const Event* event) {
  assert( event->token->ready() );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  Values data(event->token->data);
  auto evaluation = (double)extensionElements->getObjective(status,data);

  if ( 
    event->token->node->represents<BPMN::SubProcess>() ||
    event->token->node->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status,data);
  }

  if ( !extensionElements->feasibleEntry(status,data) ) {
    // entry would be infeasible
    return std::nullopt;
  }

  if ( event->token->node->represents<BPMN::Task>() && 
    !event->token->node->represents<BPMN::ReceiveTask>() &&
    !event->token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    // apply operators after checking entry restrictions and before applying guidance
    // receive tasks and decision tasks require further decision before operators are applied
    extensionElements->applyOperators(status,data);
    if ( !extensionElements->feasibleExit(status,data) ) {
      // entry would lead to infeasible exit
      return std::nullopt;
    }
  }

  if ( !extensionElements->entryGuidance ) {
    // return evaluation of entry
    return evaluation - extensionElements->getObjective(status,data);
  }
    
  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->entryGuidance->get()->apply(systemState->scenario, systemState->currentTime, event->token->owner->root->instanceId, event->token->node, status, data);
  if ( guidance.has_value() && extensionElements->feasibleExit(status,data) ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


