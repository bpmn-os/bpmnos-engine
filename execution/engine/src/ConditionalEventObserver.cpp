#include "ConditionalEventObserver.h"
#include "DataUpdate.h"
#include "model/bpmnos/src/Model.h"
#include "Engine.h"
#include "SystemState.h"
#include "Token.h"
#include "model/bpmnos/src/extensionElements/Conditions.h"
#include <algorithm>
#include <iterator>
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

  // a written global attribute, whose index is below the instance index, concerns the tokens of every
  // instance, any other written attribute only those of the instance that wrote it
  auto instanceIndex = systemState->engine->getModel()->instanceIndex;
  std::vector<const BPMNOS::Model::Attribute*> globalAttributes;
  std::ranges::copy_if(dataUpdate->attributes, std::back_inserter(globalAttributes), [instanceIndex](const BPMNOS::Model::Attribute* attribute) { return attribute->index < instanceIndex; });

  if ( !globalAttributes.empty() ) {
    // check tokens at all conditional events
    for ( auto& [instanceId,waitingTokens] : systemState->tokensAwaitingCondition ) {
      triggerConditionalEvent( instanceId == dataUpdate->instanceId ? dataUpdate->attributes : globalAttributes, waitingTokens );
    }
  }
  else {
    // check tokens at conditional events for instance with updated data
    auto it = systemState->tokensAwaitingCondition.find(dataUpdate->instanceId);
    if ( it != systemState->tokensAwaitingCondition.end() ) {
      triggerConditionalEvent( dataUpdate->attributes, it->second );      
    }
  }
}

void ConditionalEventObserver::triggerConditionalEvent(const std::vector<const BPMNOS::Model::Attribute*>& attributes, auto_list< std::weak_ptr<Token> >& waitingTokens) {
  for ( auto it = waitingTokens.begin(); it != waitingTokens.end(); ) {
    auto& [token_ptr] = *it;
    auto token = token_ptr.lock();
    assert( token->node->extensionElements->represents<BPMNOS::Model::Conditions>() );
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::Conditions>();

    // lambda determining whether data update intersects with data dependencies of conditional event
    auto intersect = [](const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) -> bool {
      for ( auto lhs : first ) {
        if ( second.contains(lhs) ) {
          return true;
        }
      }
      return false;
    };
    
    if ( intersect(attributes,extensionElements->dataDependencies) ) {
      // advance token if conditions are satisfied
      if ( extensionElements->conditionsSatisfied(token->status,*token->data) ) {
        auto engine = const_cast<Engine*>(systemState->engine);
        engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
        it = waitingTokens.erase(it);
      }
      else {
        ++it;
      }
    }
    else {
      ++it;
    }
  }
}

