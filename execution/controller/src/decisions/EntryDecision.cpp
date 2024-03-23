#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/DecisionTask.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"

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
  double evaluation = (double)extensionElements->getObjective(status);

  if ( 
    event->token->node->represents<BPMN::SubProcess>() ||
    event->token->node->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status);
  }

  if ( !extensionElements->feasibleEntry(status) ) {
    // entry would be infeasible
    return std::nullopt;
  }

  if ( event->token->node->represents<BPMN::Task>() && 
    !event->token->node->represents<BPMN::ReceiveTask>() &&
    !event->token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    // apply operators after checking entry restrictions and before applying guidance
    // receive tasks and decision tasks require further decision before operators are applied
    extensionElements->applyOperators(status);
    if ( !extensionElements->feasibleExit(status) ) {
      // entry would lead to infeasible exit
      return std::nullopt;
    }
  }

  // return evaluation of entry
  return evaluation - extensionElements->getObjective(status);
}

std::optional<double> EntryDecision::guidedEvaluator(const Event* event) {
  assert( event->token->ready() );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  auto evaluation = (double)extensionElements->getObjective(status);

  if ( 
    event->token->node->represents<BPMN::SubProcess>() ||
    event->token->node->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status);
  }

  if ( !extensionElements->feasibleEntry(status) ) {
    // entry would be infeasible
    return std::nullopt;
  }

  if ( event->token->node->represents<BPMN::Task>() && 
    !event->token->node->represents<BPMN::ReceiveTask>() &&
    !event->token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    // apply operators after checking entry restrictions and before applying guidance
    // receive tasks and decision tasks require further decision before operators are applied
    extensionElements->applyOperators(status);
    if ( !extensionElements->feasibleExit(status) ) {
      // entry would lead to infeasible exit
      return std::nullopt;
    }
  }

  if ( !extensionElements->entryGuidance ) {
    // return evaluation of entry
    return evaluation - extensionElements->getObjective(status);
  }
    
  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->entryGuidance->get()->apply(status, event->token->node, systemState->scenario, systemState->currentTime);
  if ( guidance.has_value() && extensionElements->feasibleExit(status) ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


