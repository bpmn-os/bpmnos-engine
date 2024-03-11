#include "NaiveSequentialEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

NaiveSequentialEntryHandler::NaiveSequentialEntryHandler()
{
}

void NaiveSequentialEntryHandler::subscribe(Engine* engine) {
  engine->addSubscriber(this, Execution::Observable::Type::EntryEvent);
  EventHandler::subscribe(engine);
}

std::shared_ptr<Event> NaiveSequentialEntryHandler::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, event_ptr] : sequentialEntryEvents ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      if ( auto event = event_ptr.lock() )  {
        auto tokenAtSequentialPerformer = systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get()));
        if ( !tokenAtSequentialPerformer->performing ) {
          return event;
        }
      }
    }
  }
  return nullptr;
}

void NaiveSequentialEntryHandler::notice(const Observable* observable) {
  assert(dynamic_cast<const EntryEvent*>(observable));
  auto event = const_cast<EntryEvent*>(static_cast<const EntryEvent*>(observable));
  assert(event->token->node);
  if ( event->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    sequentialEntryEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
}

