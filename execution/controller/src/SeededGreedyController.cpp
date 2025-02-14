#include "SeededGreedyController.h"
#include "dispatcher/naive/InstantEntry.h"
#include "dispatcher/naive/InstantExit.h"
#include "dispatcher/greedy/BestLimitedChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include <iostream>

using namespace BPMNOS::Execution;

SeededGreedyController::SeededGreedyController(const BPMNOS::Model::Scenario* scenario, Evaluator* evaluator)
  : CPController(scenario)
  , evaluator(evaluator)
  , _seed( CPSeed(this,CPSeed::defaultSeed(getVertices().size())) )
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<InstantEntry>() );
  eventDispatchers.push_back( std::make_unique<InstantExit>() );
  eventDispatchers.push_back( std::make_unique<BestLimitedChoice>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(evaluator) );
}

bool SeededGreedyController::setSeed(const std::list<size_t>& seed) {
  _seed = CPSeed(this,seed);
  return _seed.isFeasible();
}


void SeededGreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> SeededGreedyController::dispatchEvent(const SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
/*
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
  
//if ( best ) std::cerr << "\nSeededGreedyController: Best decision " << best->jsonify() << " evaluated with " << best->evaluation.value_or(-999) << std::endl;
*/
  return best;
}
