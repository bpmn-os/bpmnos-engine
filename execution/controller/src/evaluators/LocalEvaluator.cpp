#include "LocalEvaluator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"

using namespace BPMNOS::Execution;


bool LocalEvaluator::updateValues(EntryDecision* decision, Status& status, Data& data) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->ready() || ( token->state == Token::State::EXITING ) ); // loop activities may re-enter
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  // make sure that all initial attribute values are up to date
  extensionElements->computeInitialValues(token->owner->systemState->getTime(),status,data);

  if ( token->node->represents<BPMN::SubProcess>() ) {
    // the operators of a scope are applied to the token at its start event, i.e. after the scope is
    // entered, so its entry restrictions constrain the status they produce
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
  return extensionElements->fullScopeRestrictionsSatisfied(status,data);
}

bool LocalEvaluator::updateValues(ExitDecision* decision, Status& status, Data& data) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->completed() );

  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  }

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  return extensionElements->feasibleExit(status,data);
}

bool LocalEvaluator::updateValues(ChoiceDecision* decision, Status& status, Data& data) {
  auto token = decision->token.lock();
  assert( token );
  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  } 
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  extensionElements->applyOperators(status,data);
  return extensionElements->feasibleCompletion(status,data);
}

bool LocalEvaluator::updateValues(MessageDeliveryDecision* decision, Status& status, Data& data) {
  auto token = decision->token.lock();
  assert( token );
  // make sure that timestamp used for evaluation is up to date
  auto now = token->owner->systemState->getTime();
  if ( status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
    status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() = now;
  }
  assert( token->node->represents<BPMN::FlowNode>() );
  assert( token->node->as<BPMN::FlowNode>()->parent );
  auto extensionElements = 
    token->node->represents<BPMN::MessageStartEvent>() ?
    token->node->as<BPMN::FlowNode>()->parent->extensionElements->as<BPMNOS::Model::ExtensionElements>() :
    token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()
  ;
  assert(extensionElements);
  assert( dynamic_cast<const MessageDeliveryEvent*>(decision) );
  auto message = static_cast<const MessageDeliveryEvent*>(decision)->message.lock();
  message->apply(token->node->as<BPMN::FlowNode>(),token->getAttributeRegistry(),status,data);
  extensionElements->applyOperators(status,data);

  // check feasibility
  if ( token->node->represents<BPMN::ReceiveTask>() ) {
    return extensionElements->feasibleCompletion(status,data);
  }
  else if ( token->node->represents<BPMN::MessageStartEvent>() ) {
    return extensionElements->feasibleEntry(status,data);
  }
  else {
    return extensionElements->satisfiesInheritedRestrictions(status,data);
  }
}

std::shared_ptr<Evaluation> LocalEvaluator::evaluate(EntryDecision* decision) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->ready() || ( token->state == Token::State::EXITING ) ); // loop activities may re-enter
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Status status = token->status;
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Data data(*token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);
//std::cerr << "Initial local evaluation at node " << token->node->id << ": " << evaluation << std::endl;

  bool feasible = updateValues(decision,status,data);
  if ( !feasible ) {
    return nullptr;
  }
  // return evaluation of entry
//std::cerr << "Updated local evaluation: " << extensionElements->getObjective(status,data) << std::endl;
  return std::make_shared<Evaluation>(extensionElements->getObjective(status,data) - evaluation);
}

std::shared_ptr<Evaluation> LocalEvaluator::evaluate(ExitDecision* decision) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->completed() );
  Status status = token->status;
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Data data(*token->data);
  bool feasible = updateValues(decision,status,data);
  if ( !feasible ) {
    return nullptr;
  }
  return std::make_shared<Evaluation>(0.0);
}

std::shared_ptr<Evaluation> LocalEvaluator::evaluate(ChoiceDecision* decision) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->busy() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  assert( extensionElements->choices.size() == decision->choices.size() );
  auto evaluation = (double)extensionElements->getObjective(token->status, *token->data);

  assert( dynamic_cast<const ChoiceEvent*>(decision) );
  Status status(token->status);
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Data data(*token->data);
  // apply choices
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    extensionElements->attributeRegistry.setValue( extensionElements->choices[i]->attribute, status, data, decision->choices[i] );
  }

  bool feasible = updateValues(decision,status,data);
  if ( !feasible ) {
    return nullptr;
  }

  return std::make_shared<Evaluation>(extensionElements->getObjective(status,data) - evaluation);
}

std::shared_ptr<Evaluation> LocalEvaluator::evaluate(MessageDeliveryDecision* decision) {
  auto token = decision->token.lock();
  assert( token );
  assert( token->busy() );

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  Status status = token->status;
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = token->owner->systemState->currentTime;
  Data data(*token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);

  bool feasible = updateValues(decision,status,data);
  if ( !feasible ) {
    return nullptr;
  }

  return std::make_shared<Evaluation>(extensionElements->getObjective(status,data) - evaluation);
}


std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(EntryDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;

  auto token = decision->token.lock();
  assert( token );
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
  }
  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(ExitDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;

  auto token = decision->token.lock();
  assert( token );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  dependencies.insert(extensionElements->exitDependencies.begin(), extensionElements->exitDependencies.end());

  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(ChoiceDecision* decision) {
  auto token = decision->token.lock();
  assert( token );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);


  std::set<const BPMNOS::Model::Attribute*> dependencies = extensionElements->operatorDependencies;

  dependencies.insert(extensionElements->completionDependencies.begin(), extensionElements->completionDependencies.end());

  // add expression dependencies
  for ( auto& choice : extensionElements->choices ) {
    dependencies.insert(choice->dependencies.begin(), choice->dependencies.end());
  }
  
  return dependencies;
}

std::set<const BPMNOS::Model::Attribute*> LocalEvaluator::getDependencies(MessageDeliveryDecision* decision) {
  std::set<const BPMNOS::Model::Attribute*> dependencies;
  auto token = decision->token.lock();
  assert( token );

/* 
  // THIS SHOULD NOT BE NEEDED
  if ( token->node->represents<BPMN::SendTask>() ) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);
    dependencies.insert(extensionElements->completionDependencies.begin(), extensionElements->completionDependencies.end());
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }
*/
  if ( token->node->represents<BPMN::ReceiveTask>() ) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);
    dependencies.insert(extensionElements->completionDependencies.begin(), extensionElements->completionDependencies.end());
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }
  else if ( token->node->represents<BPMN::MessageStartEvent>() ) {
    auto eventSubProcess = token->node->as<BPMN::FlowNode>()->parent;
    auto extensionElements = eventSubProcess->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);
    dependencies.insert(extensionElements->entryDependencies.begin(), extensionElements->entryDependencies.end());
    dependencies.insert(extensionElements->operatorDependencies.begin(), extensionElements->operatorDependencies.end());
  }
  return dependencies;

}

