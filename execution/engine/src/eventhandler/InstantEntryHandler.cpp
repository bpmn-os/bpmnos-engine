#include "InstantEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntryHandler::InstantEntryHandler()
{
}

void InstantEntryHandler::subscribe(Engine* engine) {
  engine->subscribeEntryEvents(this);
  EventHandler::subscribe(engine);
}

std::shared_ptr<Event> InstantEntryHandler::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, event_ptr] : parallelEntryEvents ) {
    if ( auto token = token_ptr.lock() ) {
      if ( auto event = event_ptr.lock() )  {
        return event;
      }
    }
  }
  return nullptr;
}

void InstantEntryHandler::notice(EntryEvent* event) {
  assert(event->token->node);
  if ( !event->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    parallelEntryEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
};

/*
std::unique_ptr<Event> InstantEntryHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr] : systemState->tokensAwaitingParallelEntry ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      return std::make_unique<EntryEvent>(token.get());
    }
  }

  return nullptr;
}
*/
