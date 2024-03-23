#include "GuidedController.h"
#include "dispatcher/greedy/BestFirstParallelEntry.h"
#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/greedy/BestFirstExit.h"
#include "dispatcher/naive/RandomChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"

using namespace BPMNOS::Execution;

GuidedController::GuidedController()
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<BestFirstParallelEntry>(&EntryDecision::guidedEvaluator) );
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>(&EntryDecision::guidedEvaluator) );
  eventDispatchers.push_back( std::make_unique<BestFirstExit>(&ExitDecision::guidedEvaluator) );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() ); // TODO: best choice
  eventDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(&MessageDeliveryDecision::guidedEvaluator) );
}

void GuidedController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GuidedController::dispatchEvent(const SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
  for ( auto& eventDispatcher : eventDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      auto decision = dynamic_pointer_cast<Decision>(event);
      assert( decision );
      if ( !best ) {
        // first decision is used as best
        best = decision;
      }
      else if ( decision->evaluation.has_value() ) {
        if ( !best->evaluation.has_value() ) {
          // first evaluated decision is used as best
          best = decision;
        }
        else if ( decision->evaluation.value() < best->evaluation.value() ) {
          // decision has less costly evaluation than current best
          best = decision;
        }
      }
    }
  }

  return best;
}
