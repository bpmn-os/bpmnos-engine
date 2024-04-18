#include "GreedyController.h"
#include "dispatcher/greedy/BestFirstParallelEntry.h"
#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/naive/RandomChoice.h"  // TODO: replace with  dispatcher returning best choice
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include "dispatcher/greedy/BestFirstExit.h"
//#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator)
  : evaluator(evaluator)
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<BestFirstParallelEntry>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestFirstExit>(evaluator) );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() ); // TODO: replace with  dispatcher returning best choice
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>(evaluator) );
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
//std::cerr << "GreedyController: Decision with evaluation " << decision->evaluation.value_or(99999) <<" for: " << decision->token->jsonify() << std::endl;

        if ( !best ) {
          // first decision is used as best
          best = decision;
        }
        else if ( decision->evaluation.has_value() && !best->evaluation.has_value() ) {
          // first feasible decision is used as best
          best = decision;
        }
        else if ( decision->evaluation.has_value() && decision->evaluation.value() < best->evaluation.value() ) {
          // decision has less costly evaluation than current best
          best = decision;
        }
      }
      else {
        // events are immediately forwarded
        return event;
      }
    }
  }
/*
if ( best ) {
std::cerr << "GreedyController: Best decision with evaluation " << best->evaluation.value_or(99999) <<" for: " << best->token->jsonify() << std::endl;
}
*/
  return best;
}
