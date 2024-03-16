#include "InstantEntry.h"
#include "execution/engine/src/events/EntryDecision.h"
#include "execution/engine/src/Mediator.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntry::InstantEntry()
{
}

void InstantEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Execution::Observable::Type::EntryRequest);
  EventDispatcher::connect(mediator);
}

void InstantEntry::notice(const Observable* observable) {
  assert(dynamic_cast<const DecisionRequest*>(observable));
  auto request = static_cast<const DecisionRequest*>(observable);
  assert(request->token->node);
  if ( !request->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    parallelEntryRequests.emplace_back(request->token->weak_from_this(), request->weak_from_this());
  }
}

std::shared_ptr<Event> InstantEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr] : parallelEntryRequests ) {
    if ( auto token = token_ptr.lock() ) {
      if ( auto request = request_ptr.lock() )  {
        assert( request->token->ready() );
        return std::make_shared<EntryDecision>(token.get());
      }
    }
  }
  return nullptr;
}


