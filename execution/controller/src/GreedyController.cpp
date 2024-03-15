#include "GreedyController.h"
#include "dispatcher/naive/InstantEntry.h"
#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/naive/RandomChoice.h"
#include "dispatcher/naive/FirstMatchingMessageDelivery.h"
#include "dispatcher/naive/InstantExit.h"

using namespace BPMNOS::Execution;

GreedyController::GreedyController()
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<InstantEntry>() );
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>() );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() );
  eventDispatchers.push_back( std::make_unique<FirstMatchingMessageDelivery>() );
  eventDispatchers.push_back( std::make_unique<InstantExit>() );
//  eventDispatchers.push_back( std::make_unique<MyopicMessageTaskTerminator>() );
//  eventDispatchers.push_back( std::make_unique<MyopicDecisionTaskTerminator>() );
//  eventDispatchers.push_back( std::make_unique<TimeWarp>() );
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GreedyController::fetchEvent(SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
  for ( auto& eventDispatcher : eventDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      assert( dynamic_pointer_cast<Decision>( event ) );
      auto decision = static_pointer_cast<Decision>( event );
      if ( !best ) {
        // first decision is used as best
        best = decision;
      }
      else if (
        !best->evaluation.has_value() &&
        decision->evaluation.has_value()
      ) {
        // first decision with evaluation is used as best
        best = decision;
      }
      else if (
        best->evaluation.has_value() &&
        decision->evaluation.has_value() && 
        decision->evaluation.value() < best->evaluation.value()
      ) {
        // decision has less costly evaluation than current best
        best = decision;
      }
    }
  }
  return best;
}
