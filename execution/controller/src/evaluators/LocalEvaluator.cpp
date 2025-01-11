#include "LocalEvaluator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"

using namespace BPMNOS::Execution;


bool LocalEvaluator::updateValues(EntryDecision* decision, Values& status, Values& data, Values& globals) {
  auto token = decision->token;
  assert( token->ready() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  // make sure that all initial attribute values are up to date
  extensionElements->computeInitialValues(token->owner->systemState->getTime(),status,data,globals);

  // TODO: this shoud not be relevant
  if ( token->node->represents<BPMN::EventSubProcess>() ) {
assert(!"No entry for event-subprocesses");
    // for event-subprocesses apply operators before checking entry restrictions
    extensionElements->applyOperators(status,data,globals);
  }

  if ( !extensionElements->feasibleEntry(status,data,globals) ) {
    // entry would be infeasible
//std::cerr << "Local evaluator: std::nullopt" << std::endl;
    return false;
  }


  if ( token->node->represents<BPMN::Task>() && 
    !token->node->represents<BPMN::ReceiveTask>() &&
    !token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    // apply operators after checking entry restrictions and before applying guidance
    // receive tasks and decision tasks require further decision before operators are applied
    extensionElements->applyOperators(status,data,globals);
  }
  return true;
}

bool LocalEvaluator::updateValues(ExitDecision* decision, Values& status, Values& data, Values& globals) {
  auto token = decision->token;
  assert( token->completed() );

  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  }

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  if ( !extensionElements->feasibleExit(status,data,globals) ) {
    // exit would be infeasible
    return false;
  }

  return true;
}

bool LocalEvaluator::updateValues(ChoiceDecision* decision, Values& status, Values& data, Values& globals) {
  auto token = decision->token;
  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  } 
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  extensionElements->applyOperators(status,data,globals);
  // TODO: do we want to check feasibility here?  
  if ( !extensionElements->fullScopeRestrictionsSatisfied(status,data,globals) ) {
    // exit would be infeasible
    return false;
  }

  return true;
}

bool LocalEvaluator::updateValues(MessageDeliveryDecision* decision, Values& status, Values& data, Values& globals) {
  auto token = decision->token;
  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  } 
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  assert( dynamic_cast<const MessageDeliveryEvent*>(decision) );
  auto message = static_cast<const MessageDeliveryEvent*>(decision)->message.lock();
  message->apply(token->node,token->getAttributeRegistry(),status,data,globals);
  extensionElements->applyOperators(status,data,globals);

  // TODO: do we want to check feasibility here?  
  if ( !extensionElements->fullScopeRestrictionsSatisfied(status,data,globals) ) {
    // exit would be infeasible
    return false;
  }

  return true;
}

std::optional<double> LocalEvaluator::evaluate(EntryDecision* decision) {
  auto token = decision->token;
  assert( token->ready() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  Values globals = token->globals;
  double evaluation = (double)extensionElements->getObjective(status,data,globals);
//std::cerr << "Initial local evaluation at node " << token->node->id << ": " << evaluation << std::endl;

  bool feasible = updateValues(decision,status,data,globals); 
  if ( !feasible ) {
    return std::nullopt;
  }
  // return evaluation of entry
//std::cerr << "Updated local evaluation: " << extensionElements->getObjective(status,data,globals) << std::endl;
  return evaluation - extensionElements->getObjective(status,data,globals);
}

std::optional<double> LocalEvaluator::evaluate(ExitDecision* decision) {
  auto token = decision->token;
  assert( token->completed() );
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  Values globals = token->globals;
  bool feasible = updateValues(decision,status,data,globals); 
  if ( !feasible ) {
    return std::nullopt;
  }
  return 0;
}

std::optional<double> LocalEvaluator::evaluate(ChoiceDecision* decision) {
  auto token = decision->token;
  assert( token->busy() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  assert( extensionElements->choices.size() == decision->choices.size() );
  auto evaluation = (double)extensionElements->getObjective(token->status, *token->data, token->globals);

  assert( dynamic_cast<const ChoiceEvent*>(decision) );
  Values status(token->status);
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  Values globals = token->globals;
  // apply choices
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    extensionElements->attributeRegistry.setValue( extensionElements->choices[i]->attribute, status, data, globals, decision->choices[i] );
  }

  bool feasible = updateValues(decision,status,data,globals); 
  if ( !feasible ) {
    return std::nullopt;
  }

  return evaluation - extensionElements->getObjective(status,data,globals);
}

std::optional<double> LocalEvaluator::evaluate(MessageDeliveryDecision* decision) {
  auto token = decision->token;
  assert( token->busy() );

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  Values globals = token->globals;
  double evaluation = (double)extensionElements->getObjective(status,data,globals);

  bool feasible = updateValues(decision,status,data,globals); 
  if ( !feasible ) {
    return std::nullopt;
  }

  return evaluation - extensionElements->getObjective(status,data,globals);
}


std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(EntryDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;

  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  if ( 
    token->node->represents<BPMN::Activity>() &&
    !token->node->represents<BPMN::Task>()
  ) {
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }

  dependencies.insert(extensionElements->entryDependencies.begin(), extensionElements->entryDependencies.end());

  if ( token->node->represents<BPMN::Task>() && 
    !token->node->represents<BPMN::ReceiveTask>() &&
    !token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
//    dependencies.insert(extensionElements->exitDependencies.begin(), extensionElements->exitDependencies.end());
  }
  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(ExitDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;

  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  dependencies.insert(extensionElements->exitDependencies.begin(), extensionElements->exitDependencies.end());

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(ChoiceDecision* decision) {
  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = extensionElements->operatorDependencies;

  // add expression dependencies
  for ( auto& choice : extensionElements->choices ) {
    dependencies.insert(choice->dependencies.begin(), choice->dependencies.end());
  }
  
  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(MessageDeliveryDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;
  auto token = decision->token;

  if ( token->node->represents<BPMN::SendTask>() ) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }
  else if ( token->node->represents<BPMN::MessageStartEvent>() ) {
    auto eventSubProcess = token->node->parent;
    auto extensionElements = eventSubProcess->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);
    dependencies.insert(extensionElements->entryDependencies.begin(), extensionElements->entryDependencies.end());
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }
  return dependencies;

}

