#include "GreedyController.h"
//#include "dispatcher/greedy/BestFirstParallelEntry.h"
//#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/greedy/BestFirstEntry.h"
#include "dispatcher/greedy/BestLimitedChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include "dispatcher/greedy/BestFirstExit.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator)
  : evaluator(evaluator)
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<BestFirstEntry>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestFirstExit>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestLimitedChoice>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(evaluator) );
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
      if (  auto decision = dynamic_pointer_cast<Decision>(event) ) {
        if ( decision->evaluation.has_value() ) {
          if ( !best ) {
            // first feasible decision is used as best
            best = decision;
          }
          else if ( decision->evaluation.value() < best->evaluation.value() ) {
            // decision has less costly evaluation than current best
            best = decision;
          }
        }
      }
      else {
        // events are immediately forwarded
//std::cerr << "\nEvent " << event->jsonify() << " without evaluation" << std::endl;
        return event;
      }
    }
  }
  
//if ( best ) std::cerr << "\nGreedyController: Best decision " << best->jsonify() << " evaluated with " << best->evaluation.value_or(-999) << std::endl;

  return best;
}
