#include "BestLimitedChoice.h"
#include "execution/engine/src/Mediator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include "model/bpmnos/src/extensionElements/expression/LinearExpression.h"
#include "model/bpmnos/src/extensionElements/expression/Enumeration.h"
#include "model/utility/src/CollectionRegistry.h"
#include <cassert>
#include <ranges>
//#include <iostream>

using namespace BPMNOS::Execution;

BestLimitedChoice::BestLimitedChoice(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
{
}

void BestLimitedChoice::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::ChoiceRequest
  );
  GreedyDispatcher::connect(mediator);
}


void BestLimitedChoice::notice(const Observable* observable) {
//std::cout << "BestLimitedChoice::notice" << std::endl;
  if ( observable->getObservableType() == Observable::Type::ChoiceRequest ) {
//std::cout << "Choice requested" << std::endl;
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    requestsWithoutDecisions.emplace_back( request->token->weak_from_this(), request->weak_from_this() );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

std::vector< BPMNOS::Values > BestLimitedChoice::determineDecisions(const Token* token ) {
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
//std::cout <<  "BestLimitedChoice::determineDecisions: " << token->jsonify() << std::endl;

  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  
  
  assert( extensionElements->choices.size() );
      
  auto status = token->status;
  auto data = *token->data;
  auto globals = token->globals;
  // initialize decisions with an empty set of choices
  std::vector< BPMNOS::Values > decisions = { {} };
  // iterate through all choices to be made
  for ( auto& choice : extensionElements->choices ) {
    std::vector< BPMNOS::Values > updatedDecisions;
    // iterate through all partial decisions and make next choice
    for ( auto& choices : decisions ) {
//std::cout << "apply previous choices..." << std::endl;
      // apply previous choices
      for ( size_t i = 0; i < choices.size(); i++ ) {
        auto& prior_choice = extensionElements->choices[i];
        prior_choice->attributeRegistry.setValue(choice->attribute, status, data, globals, choices[i]);
      }
      
      // make new choices
      std::vector<BPMNOS::number> alternatives;
//std::cout << "getEnumeration..." << std::endl;
      
      if ( auto allowedValues = choice->getEnumeration(status,data,globals); allowedValues.has_value() ) {
        auto& values = allowedValues.value();
/*
        if ( values.size() ) {
          if ( choice->attribute->type != STRING ) {
            auto [lb,ub] = choice->getBounds(status,data,globals);
            // remove values outside of bounds
            values.erase(
              std::remove_if(
                values.begin(),
                values.end(),
                [&](const BPMNOS::number& value) { return (value < lb || value > ub); }
              ),
              values.end()
            );
          }
          for ( auto value : values ) {
            alternatives.push_back( value );
          }
        }
*/
        for ( auto value : values ) {
          alternatives.push_back( value );
        }
      }
      else if ( choice->attribute->type != STRING ) {
        auto [min,max] = choice->getBounds(status,data,globals);
//std::cout << "bounds = [" << min << "," << max << "]" << std::endl;
        if ( min == max ) {
          alternatives.push_back( min );
        }
        else if ( min < max ) {
          alternatives.push_back( min );
          alternatives.push_back( max );
        }
      }
      
//std::cout << alternatives.size() << " alternatives..." << std::endl;
      for ( auto value : alternatives ) {
        // add alternative to prior choices
        BPMNOS::Values updatedChoices = choices;      
        updatedChoices.push_back( value );
        updatedDecisions.push_back(std::move(updatedChoices));
      }
    }
//std::cout << "update decisions..." << std::endl;
    // update decisions
    decisions = std::move(updatedDecisions);
    // continue with next choice to be made
  }
//std::cout << decisions.size() << " decisions..." << std::endl;
  return decisions;
}


std::shared_ptr<Event> BestLimitedChoice::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "BestLimitedChoice::dispatchEvent" << std::endl;
  for ( auto& [ token_ptr, request_ptr, decision ] : decisionsWithoutEvaluation ) {
    auto token = token_ptr.lock();
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( 
      std::ranges::any_of(
        extensionElements->restrictions, [](const std::unique_ptr<BPMNOS::Model::Restriction>& restriction) {
          return restriction->scope != BPMNOS::Model::Restriction::Scope::ENTRY && !dynamic_cast<BPMNOS::Model::Enumeration*>(restriction->expression.get());
        }
      )
    ) {
      // choices that are not explicitly enumerated may have to be changed
      // decisions containing such choices are deleted and recreated
      auto request = request_ptr.lock();
      requestsWithoutDecisions.emplace_back( request->token->weak_from_this(), request->weak_from_this() );
      continue;
    }

    assert(decision);
    if ( decision ) {
//std::cerr << "Re-evaluate choice decision: " << decision->jsonify().dump() << std::endl;
      evaluate( token_ptr, request_ptr, std::move(decision) );
    }
  }
  decisionsWithoutEvaluation.clear();
  
  for ( auto& [ token_ptr, request_ptr ] : requestsWithoutDecisions ) {
    auto request = request_ptr.lock();
    for ( auto choices : determineDecisions( request->token ) ) {
      auto decision = std::make_shared<ChoiceDecision>(request->token, choices, evaluator);
//std::cerr << "Make and evaluate choice decision: " << decision->jsonify().dump() << std::endl;
      evaluate( token_ptr, request_ptr, std::move(decision) );
//std::cerr << "Decision evaluated" << std::endl;
    }
  }
  requestsWithoutDecisions.clear();

  for ( auto [ cost, token_ptr, request_ptr, event_ptr ] : evaluatedDecisions ) {
    // return best evaluated decision
//std::cerr << "Best choice decision " << event_ptr.lock()->jsonify() << " evaluated with " << cost << std::endl;
    return event_ptr.lock();
  }

//std::cerr << "No evaluated choice decision" << std::endl;
  return nullptr;
}

