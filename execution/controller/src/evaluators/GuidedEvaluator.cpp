#include "GuidedEvaluator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"

using namespace BPMNOS::Execution;


bool GuidedEvaluator::updateValues(EntryDecision* decision, Values& status, Values& data, Values& globals) {
  if ( !LocalEvaluator::updateValues(decision,status,data,globals) ) {
    return false;
  }

  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  if ( !extensionElements->entryGuidance ) {
    return true;
  }

  // apply guidance
  auto guidance = extensionElements->entryGuidance.value().get();
  auto systemState = decision->token->owner->systemState;
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data, globals);
  
  return guidance->restrictionsSatisfied(decision->token->node,status,data,globals);
}

bool GuidedEvaluator::updateValues(ExitDecision* decision, Values& status, Values& data, Values& globals) {
  if ( !LocalEvaluator::updateValues(decision,status,data,globals) ) {
    return false;
  }

  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  if ( !extensionElements->exitGuidance ) {
    return true;
  }

  // apply guidance
  auto guidance = extensionElements->exitGuidance.value().get();
  auto systemState = decision->token->owner->systemState;
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data, globals);

  return guidance->restrictionsSatisfied(decision->token->node,status,data,globals);
}


bool GuidedEvaluator::updateValues(ChoiceDecision* decision, Values& status, Values& data, Values& globals) {
  if ( !LocalEvaluator::updateValues(decision,status,data,globals) ) {
    return false;
  }

  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  if ( !extensionElements->choiceGuidance ) {
    return true;
  }

  // apply guidance
  auto guidance = extensionElements->choiceGuidance.value().get();
  auto systemState = decision->token->owner->systemState;
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data, globals);

  return guidance->restrictionsSatisfied(decision->token->node,status,data,globals);
}


bool GuidedEvaluator::updateValues(MessageDeliveryDecision* decision, Values& status, Values& data, Values& globals) {
  if ( !LocalEvaluator::updateValues(decision,status,data,globals) ) {
    return false;
  }

  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  if ( !extensionElements->messageDeliveryGuidance ) {
    return true;
  }

  // apply guidance
  auto guidance = extensionElements->messageDeliveryGuidance.value().get();
  auto systemState = decision->token->owner->systemState;
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data, globals);

  return guidance->restrictionsSatisfied(decision->token->node,status,data,globals);
}

std::optional<double> GuidedEvaluator::evaluate(EntryDecision* decision) {
  auto token = decision->token;
  assert( token->ready() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Values status = token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Values data(*token->data);
  Values globals = token->globals;
  double evaluation = (double)extensionElements->getObjective(status,data,globals);
//std::cerr << "Initial evaluation: " << evaluation << std::endl;

  bool feasible = updateValues(decision,status,data,globals); 
  if ( !feasible ) {
    return std::nullopt;
  }

  if ( !extensionElements->entryGuidance ) {
    return evaluation - extensionElements->getObjective(status,data,globals);
  }
  // return evaluation of entry
//std::cerr << "Updated evaluation: " << extensionElements->getObjective(status,data,globals) << std::endl;
  return evaluation - extensionElements->entryGuidance.value()->getObjective(status,data,globals);
}

std::optional<double> GuidedEvaluator::evaluate(ExitDecision* decision) {
  auto token = decision->token;
  assert( token->completed() );
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

  if ( !extensionElements->exitGuidance ) {
    return evaluation - extensionElements->getObjective(status,data,globals);
  }

  return evaluation - extensionElements->exitGuidance.value()->getObjective(status,data,globals);
}

std::optional<double> GuidedEvaluator::evaluate(ChoiceDecision* decision) {
  auto token = decision->token;
  assert( token->busy() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
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

  if ( !extensionElements->choiceGuidance ) {
    return evaluation - extensionElements->getObjective(status,data,globals);
  }

  return evaluation - extensionElements->choiceGuidance.value()->getObjective(status,data,globals);
}

std::optional<double> GuidedEvaluator::evaluate(MessageDeliveryDecision* decision) {
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

  if ( !extensionElements->messageDeliveryGuidance ) {
    return evaluation - extensionElements->getObjective(status,data,globals);
  }

  return evaluation - extensionElements->messageDeliveryGuidance.value()->getObjective(status,data,globals);
}


std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(EntryDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->entryGuidance.has_value() ) {
    dependencies.insert(extensionElements->entryGuidance.value()->dependencies.begin(), extensionElements->entryGuidance.value()->dependencies.end());
  }

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(ExitDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->exitGuidance.has_value() ) {
    dependencies.insert(extensionElements->exitGuidance.value()->dependencies.begin(), extensionElements->exitGuidance.value()->dependencies.end());
  }

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(ChoiceDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->choiceGuidance.has_value() ) {
    dependencies.insert(extensionElements->choiceGuidance.value()->dependencies.begin(), extensionElements->choiceGuidance.value()->dependencies.end());
  }

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(MessageDeliveryDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->messageDeliveryGuidance.has_value() ) {
    dependencies.insert(extensionElements->messageDeliveryGuidance.value()->dependencies.begin(), extensionElements->messageDeliveryGuidance.value()->dependencies.end());
  }

  return dependencies;
}

