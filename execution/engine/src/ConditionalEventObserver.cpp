#include "ConditionalEventObserver.h"
#include "Engine.h"
#include "SystemState.h"
#include "Token.h"
#include "model/bpmnos/src/extensionElements/Conditions.h"
#include <iostream>

using namespace BPMNOS::Execution;

ConditionalEventObserver::ConditionalEventObserver() : systemState(nullptr) {
}

void ConditionalEventObserver::connect(SystemState* systemState) {
  this->systemState = systemState;
}

void ConditionalEventObserver::notice(const Observable* observable) {
  assert( systemState );
  assert( dynamic_cast<const DataUpdate*>(observable) );
  auto dataUpdate = static_cast<const DataUpdate*>(observable);

  if ( dataUpdate->global() ) {
    // check tokens at all conditional events
    for ( auto& [_,waitingTokens] : systemState->tokensAwaitingCondition ) {
      triggerConditionalEvent( dataUpdate, waitingTokens );
    }
  }
  else {
    // check tokens at conditional events for instance with updated data
    auto it = systemState->tokensAwaitingCondition.find(dataUpdate->instanceId);
    if ( it != systemState->tokensAwaitingCondition.end() ) {
      triggerConditionalEvent( dataUpdate, it->second );      
    }
  }
}

void ConditionalEventObserver::triggerConditionalEvent(const DataUpdate* dataUpdate, auto_list< std::weak_ptr<Token> >& waitingTokens) {
  for ( auto it = waitingTokens.begin(); it != waitingTokens.end(); ) {
    auto& [token_ptr] = *it;
    auto token = token_ptr.lock();
    assert( token->node->extensionElements->represents<BPMNOS::Model::Conditions>() );
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::Conditions>();

    // determine whether data update intersects with data dependencies of conditional event
    auto intersect = [](const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) -> bool {
      for ( auto lhs : first ) {
        if ( second.contains(lhs) ) {
          return true;
        }
      }
      return false;
    };
    
    if ( intersect(dataUpdate->attributes,extensionElements->dataDependencies) ) {
      // advance token if conditions are satisfied
      if ( extensionElements->conditionsSatisfied(token->status,*token->data,token->globals) ) {
        auto engine = const_cast<Engine*>(systemState->engine);
        engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
        it = waitingTokens.erase(it);
      }
      else {
        ++it;
      }
    }
  }
}

