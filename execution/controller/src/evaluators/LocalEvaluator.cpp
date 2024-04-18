#include "LocalEvaluator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"

using namespace BPMNOS::Execution;


bool LocalEvaluator::updateValues(EntryDecision* decision, Values& status, Values& data) {
  auto token = decision->token;
  assert( token->ready() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  if ( 
    token->node->represents<BPMN::Activity>() &&
    !token->node->represents<BPMN::Task>()
  ) {
    // apply operators before checking entry restrictions
    extensionElements->applyOperators(status,data);
  }

  if ( !extensionElements->feasibleEntry(status,data) ) {
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
    extensionElements->applyOperators(status,data);
  }
  return true;
}

bool LocalEvaluator::updateValues(ExitDecision* decision, Values& status, Values& data) {
  auto token = decision->token;
  assert( token->completed() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  if ( !extensionElements->feasibleExit(status,data) ) {
    // exit would be infeasible
    return false;
  }

  return true;
}

bool LocalEvaluator::updateValues(ChoiceDecision* decision, Values& status, Values& data) {
  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  extensionElements->applyOperators(status,data);
  // TODO: do we want to check feasibility here?  
  if ( !extensionElements->fullScopeRestrictionsSatisfied(status,data) ) {
    // exit would be infeasible
    return false;
  }

  return true;
}

bool LocalEvaluator::updateValues(MessageDeliveryDecision* decision, Values& status, Values& data) {
  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  assert( dynamic_cast<const MessageDeliveryEvent*>(decision) );
  auto message = static_cast<const MessageDeliveryEvent*>(decision)->message.lock();
  message->apply(token->node,token->getAttributeRegistry(),status,data);
  extensionElements->applyOperators(status,data);

  // TODO: do we want to check feasibility here?  
  if ( !extensionElements->fullScopeRestrictionsSatisfied(status,data) ) {
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
  double evaluation = (double)extensionElements->getObjective(status,data);
//std::cerr << "Initial evaluation: " << evaluation << std::endl;

  bool feasible = updateValues(decision,status,data); 
  if ( !feasible ) {
    return std::nullopt;
  }
  // return evaluation of entry
//std::cerr << "Updated evaluation: " << extensionElements->getObjective(status,data) << std::endl;
  return evaluation - extensionElements->getObjective(status,data);
}

std::optional<double> LocalEvaluator::evaluate(ExitDecision* decision) {
  auto token = decision->token;
  assert( token->completed() );
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  bool feasible = updateValues(decision,status,data); 
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
  auto evaluation = (double)extensionElements->getObjective(token->status, *token->data);

  assert( dynamic_cast<const ChoiceEvent*>(decision) );
  Values status = static_cast<const ChoiceEvent*>(decision)->updatedStatus;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);

  bool feasible = updateValues(decision,status,data); 
  if ( !feasible ) {
    return std::nullopt;
  }

  return evaluation - extensionElements->getObjective(status,data);
}

std::optional<double> LocalEvaluator::evaluate(MessageDeliveryDecision* decision) {
  auto token = decision->token;
  assert( token->busy() );

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);

  bool feasible = updateValues(decision,status,data); 
  if ( !feasible ) {
    return std::nullopt;
  }

  return evaluation - extensionElements->getObjective(status,data);
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
  return extensionElements->operatorDependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(MessageDeliveryDecision* decision) {
  auto token = decision->token;
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  return extensionElements->operatorDependencies;
}

