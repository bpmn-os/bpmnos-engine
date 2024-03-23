#include "BestFirstExit.h"
#include "execution/engine/src/Engine.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstExit::BestFirstExit( std::function<std::optional<double>(const Event* event)> evaluator )
  : evaluator(evaluator)
{
}

void BestFirstExit::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::ExitRequest
  );
  EventDispatcher::connect(mediator);
}

std::shared_ptr<Event> BestFirstExit::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
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

void BestFirstExit::notice(const Observable* observable) {
  assert( dynamic_cast<const DecisionRequest*>(observable) );
  auto request = static_cast<const DecisionRequest*>(observable);

  auto token = const_cast<Token*>(request->token);
  auto decision = std::make_shared<ExitDecision>(token, evaluator);
  decisions.emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), token->weak_from_this(), request->weak_from_this(), decision );
}


