#include "BestFirstParallelEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstParallelEntry::BestFirstParallelEntry( std::function<std::optional<double>(const Event* event)> evaluator )
  : evaluator(evaluator)
{
}

void BestFirstParallelEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest
  );
  EventDispatcher::connect(mediator);
}

std::shared_ptr<Event> BestFirstParallelEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto [ cost, token_ptr, request_ptr, decision ] : decisions ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      if ( request_ptr.lock() && decision->evaluation.has_value() )  {
        // request is still valid and evaluation of decision gave a value
        return decision;
      }
    }
  }
  return nullptr;
}

void BestFirstParallelEntry::notice(const Observable* observable) {
  assert( dynamic_cast<const DecisionRequest*>(observable) );
  auto request = static_cast<const DecisionRequest*>(observable);
  assert(request->token->node);
  if ( request->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    return;
  }

  auto token = const_cast<Token*>(request->token);
  auto decision = std::make_shared<EntryDecision>(token, evaluator);
  decisions.emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), token->weak_from_this(), request->weak_from_this(), decision );
}


