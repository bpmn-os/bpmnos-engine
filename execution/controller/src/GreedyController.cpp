#include "GreedyController.h"
#include "dispatcher/greedy/GreedyEntry.h"
#include "dispatcher/greedy/GreedyExit.h"
#include "dispatcher/greedy/BestFirstEntry.h"
#include "dispatcher/greedy/BestFirstExit.h"
#include "dispatcher/greedy/BestLimitedChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator, Config config)
  : evaluator(evaluator)
  , config(config)
{
  // add event dispatcher
  if ( config.bestFirstEntry ) {
    eventDispatchers.push_back( std::make_unique<BestFirstEntry>(evaluator) );
  }
  else {
    eventDispatchers.push_back( std::make_unique<GreedyEntry>(evaluator) );
  }
  if ( config.bestFirstExit ) {
    eventDispatchers.push_back( std::make_unique<BestFirstExit>(evaluator) );
  }
  else {
    eventDispatchers.push_back( std::make_unique<GreedyExit>(evaluator) );
  }
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
        if ( decision->reward.has_value() ) {
          if ( !best ) {
            // first feasible decision is used as best
            best = decision;
          }
          else if ( decision->reward.value() > best->reward.value() ) {
            // decision has better reward than current best
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
