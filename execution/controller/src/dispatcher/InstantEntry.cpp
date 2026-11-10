#include "InstantEntry.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/engine/src/Token.h"
#include "model/bpmnos/src/Model.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntry::InstantEntry()
{
}

std::shared_ptr<Event> InstantEntry::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr] : systemState->pendingEntryDecisions ) {
    if( auto request = request_ptr.lock() )  {
      assert( BPMNOS::Model::Model::isSequentialActivity(request->token->node) );
      return std::make_shared<EntryEvent>(request->token);
    }
  }
  return nullptr;
}

