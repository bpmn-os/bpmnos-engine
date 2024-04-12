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

  if (*evaluator.target<std::optional<double> (*)(const BPMNOS::Execution::Event*)>() == &EntryDecision::localEvaluator) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( 
      token->node->represents<BPMN::Activity>() &&
      !token->node->represents<BPMN::Task>()
    ) {
      determineDependencies( extensionElements->operatorDependencies );
    }
    determineDependencies( extensionElements->entryDependencies );

    if ( token->node->represents<BPMN::Task>() && 
      !token->node->represents<BPMN::ReceiveTask>() &&
      !token->node->represents<BPMNOS::Model::DecisionTask>()
    ) {
      determineDependencies( extensionElements->operatorDependencies );
      determineDependencies( extensionElements->exitDependencies );
    }
  }
  else if (*evaluator.target<std::optional<double> (*)(const BPMNOS::Execution::Event*)>() == &EntryDecision::guidedEvaluator) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( 
      token->node->represents<BPMN::Activity>() &&
      !token->node->represents<BPMN::Task>()
    ) {
      determineDependencies( extensionElements->operatorDependencies );
    }
    determineDependencies( extensionElements->entryDependencies );

    if ( token->node->represents<BPMN::Task>() && 
      !token->node->represents<BPMN::ReceiveTask>() &&
      !token->node->represents<BPMNOS::Model::DecisionTask>()
    ) {
      determineDependencies( extensionElements->operatorDependencies );
      determineDependencies( extensionElements->exitDependencies );
    }

    if ( extensionElements->entryGuidance.has_value() ) {
      determineDependencies( extensionElements->entryGuidance.value()->dependencies );
    }
  }
}

std::optional<double> EntryDecision::evaluate() {
  evaluation = evaluator((EntryEvent*)this);
//std::cerr << "Entry decision has value: " << (evaluation ? evaluation.value() : std::numeric_limits<double>::max()) << std::endl;
  return evaluation;
}

std::optional<double> EntryDecision::localEvaluator(const Event* event) {
//std::cerr << "Local evaluator : " << event->token->jsonify() << std::endl;
  assert( event->token->ready() );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  Values data(*event->token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);
//std::cerr << "Initial evaluation: " << evaluation << std::endl;

  if ( 
    event->token->node->represents<BPMN::Activity>() &&
    !event->token->node->represents<BPMN::Task>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status,data);
  }

  if ( !extensionElements->feasibleEntry(status,data) ) {
    // entry would be infeasible
//std::cerr << "Local evaluator: std::nullopt" << std::endl;
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
//std::cerr << "Updated evaluation: " << extensionElements->getObjective(status,data) << std::endl;
  return evaluation - extensionElements->getObjective(status,data);
}

std::optional<double> EntryDecision::guidedEvaluator(const Event* event) {
//std::cerr << "Guided evaluator : " << event->token->jsonify() << std::endl;
  assert( event->token->ready() );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  Values data(*event->token->data);
  auto evaluation = (double)extensionElements->getObjective(status,data);

  if ( 
    event->token->node->represents<BPMN::Activity>() &&
    !event->token->node->represents<BPMN::Task>()
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
  auto guidance = extensionElements->entryGuidance->get()->apply(systemState->scenario, systemState->currentTime, event->token->owner->root->instance.value(), event->token->node, status, data);
  if ( guidance.has_value() && extensionElements->feasibleExit(status,data) ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


