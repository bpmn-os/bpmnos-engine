#include "GreedyController.h"
#include "dispatcher/naive/InstantEntry.h"
#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/naive/RandomChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include "dispatcher/naive/InstantExit.h"

using namespace BPMNOS::Execution;

GreedyController::GreedyController()
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<InstantEntry>() );
  eventDispatchers.push_back( std::make_unique<InstantExit>() );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() );
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>(&EntryDecision::localEvaluator) );
  eventDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(&MessageDeliveryDecision::localEvaluator) );
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GreedyController::dispatchEvent(const SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
  for ( auto& eventDispatcher : eventDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      auto decision = dynamic_pointer_cast<Decision>(event);
      assert( decision );
      if ( !decision->evaluation.has_value() ) {
        // decisions that are not evaluated are immediately forwarded
        return decision;
      }
      else if ( !best ) {
        // first decision is used as best
        best = decision;
      }
      else if ( decision->evaluation.value() < best->evaluation.value() ) {
        // decision has less costly evaluation than current best
        best = decision;
      }
    }
  }

  return best;
}
