#include "GuidedEvaluator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"

using namespace BPMNOS::Execution;


bool GuidedEvaluator::updateValues(EntryDecision* decision, Values& status, Values& data) {
  if ( !LocalEvaluator::updateValues(decision,status,data) ) {
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
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data);
  
  return guidance->restrictionsSatisfied(decision->token->node,status,data);
}

bool GuidedEvaluator::updateValues(ExitDecision* decision, Values& status, Values& data) {
  if ( !LocalEvaluator::updateValues(decision,status,data) ) {
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
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data);

  return guidance->restrictionsSatisfied(decision->token->node,status,data);
}


bool GuidedEvaluator::updateValues(ChoiceDecision* decision, Values& status, Values& data) {
  if ( !LocalEvaluator::updateValues(decision,status,data) ) {
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
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data);

  return guidance->restrictionsSatisfied(decision->token->node,status,data);
}


bool GuidedEvaluator::updateValues(MessageDeliveryDecision* decision, Values& status, Values& data) {
  if ( !LocalEvaluator::updateValues(decision,status,data) ) {
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
  guidance->apply(systemState->scenario, systemState->currentTime, decision->token->owner->root->instance.value(), decision->token->node, status, data);

  return guidance->restrictionsSatisfied(decision->token->node,status,data);
}



std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(EntryDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->choiceGuidance.has_value() ) {
    dependencies.insert(extensionElements->entryGuidance.value()->dependencies.begin(), extensionElements->entryGuidance.value()->dependencies.end());
  }

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> GuidedEvaluator::getDependencies(ExitDecision* decision) {
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  std::set<const BPMNOS::Model::Attribute*> dependencies = LocalEvaluator::getDependencies(decision);
  // add guidance dependencies
  if ( extensionElements->choiceGuidance.has_value() ) {
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
  if ( extensionElements->choiceGuidance.has_value() ) {
    dependencies.insert(extensionElements->messageDeliveryGuidance.value()->dependencies.begin(), extensionElements->messageDeliveryGuidance.value()->dependencies.end());
  }

  return dependencies;
}

