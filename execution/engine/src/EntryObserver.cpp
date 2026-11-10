#include "EntryObserver.h"
#include "DataUpdate.h"
#include "SystemState.h"
#include "StateMachine.h"
#include "Token.h"
#include "events/EntryEvent.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>

using namespace BPMNOS::Execution;

EntryObserver::EntryObserver()
  : systemState(nullptr)
  , time(std::numeric_limits<BPMNOS::number>::lowest())
{
}

void EntryObserver::connect(SystemState* systemState) {
  this->systemState = systemState;
  time = systemState->getTime();
  candidates.clear();
  // the tokens awaiting entry in an installed state are checked again, since the engine whose state was copied
  // need not have checked them before the copy was made
  for ( auto& [instanceId,waitingTokens] : systemState->tokensAwaitingEntry ) {
    for ( auto& [token_ptr] : waitingTokens ) {
      candidates.emplace_back(token_ptr);
    }
  }
}

void EntryObserver::await(Token* token) {
  assert( systemState );
  assert( !BPMNOS::Model::Model::isSequentialActivity(token->node) );
  systemState->tokensAwaitingEntry[token->owner->root->instance.value()].emplace_back(token->weak_from_this());
  candidates.emplace_back(token->weak_from_this());
}

void EntryObserver::notice(const Observable* observable) {
  assert( systemState );
  assert( dynamic_cast<const DataUpdate*>(observable) );
  auto dataUpdate = static_cast<const DataUpdate*>(observable);

  // a written global attribute or object concerns the tokens of every instance, any other written attribute
  // only those of the instance that wrote it
  std::vector<const BPMNOS::Model::Attribute*> globalAttributes;
  std::ranges::copy_if(dataUpdate->attributes, std::back_inserter(globalAttributes), [](const BPMNOS::Model::Attribute* attribute) { return attribute->isGlobal; });

  if ( !globalAttributes.empty() ) {
    for ( auto& [instanceId,waitingTokens] : systemState->tokensAwaitingEntry ) {
      schedule( instanceId == dataUpdate->instanceId ? dataUpdate->attributes : globalAttributes, waitingTokens );
    }
  }
  else if ( auto it = systemState->tokensAwaitingEntry.find(dataUpdate->instanceId); it != systemState->tokensAwaitingEntry.end() ) {
    schedule( dataUpdate->attributes, it->second );
  }
}

void EntryObserver::schedule(const std::vector<const BPMNOS::Model::Attribute*>& attributes, auto_list< std::weak_ptr<Token> >& waitingTokens) {
  for ( auto& [token_ptr] : waitingTokens ) {
    auto token = token_ptr.lock();
    assert( token );
    auto& dependencies = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->entryDependencies;
    if ( std::ranges::any_of(attributes, [&dependencies](const BPMNOS::Model::Attribute* attribute) { return dependencies.contains(attribute); }) ) {
      candidates.emplace_back(token_ptr);
    }
  }
}

bool EntryObserver::isTimeDependent(const Token* token) {
  auto& dependencies = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->entryDependencies;
  return std::ranges::any_of(dependencies, [](const BPMNOS::Model::Attribute* attribute) {
    return
      attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
      !attribute->isObject() &&
      attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
    ;
  });
}

std::shared_ptr<Event> EntryObserver::dispatchEvent() {
  assert( systemState );

  if ( systemState->getTime() != time ) {
    // time has advanced, so that restrictions depending on the timestamp may hold now
    time = systemState->getTime();
    for ( auto& [instanceId,waitingTokens] : systemState->tokensAwaitingEntry ) {
      for ( auto& [token_ptr] : waitingTokens ) {
        if ( auto token = token_ptr.lock(); token && isTimeDependent(token.get()) ) {
          candidates.emplace_back(token_ptr);
        }
      }
    }
  }

  auto it = candidates.begin();
  while ( it != candidates.end() ) {
    auto token = std::get<0>(*it).lock();
    it = candidates.erase(it);
    assert( token );
    auto& waitingTokens = systemState->tokensAwaitingEntry[token->owner->root->instance.value()];
    if ( token->state != Token::State::READY ) {
      // the token no longer awaits entry
      waitingTokens.remove(token.get());
      continue;
    }
    if ( token->entryConditionHolds() ) {
      waitingTokens.remove(token.get());
      return std::make_shared<EntryEvent>(token.get());
    }
  }
  return nullptr;
}
