#include "InstantEntry.h"
#include "execution/engine/src/events/EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntry::InstantEntry()
{
}

void InstantEntry::subscribe(Engine* engine) {
  engine->addSubscriber(this, Execution::Observable::Type::EntryRequest);
  EventDispatcher::subscribe(engine);
}

std::shared_ptr<Event> InstantEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, event_ptr] : parallelEntryEvents ) {
    if ( auto token = token_ptr.lock() ) {
      if ( auto event = event_ptr.lock() )  {
        return event;
      }
    }
  }
  return nullptr;
}

void InstantEntry::notice(const Observable* observable) {
  assert(dynamic_cast<const EntryDecision*>(observable));
  auto event = const_cast<EntryDecision*>(static_cast<const EntryDecision*>(observable));
  assert(event->token->node);
  if ( !event->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    parallelEntryEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
}
