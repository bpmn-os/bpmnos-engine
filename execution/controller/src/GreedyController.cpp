#include "GreedyController.h"
#include "execution/engine/src/Engine.h"
#include "GreedyDispatcher.h"
#include "candidates/FirstFeasibleExit.h"
#include "candidates/FirstFeasibleEntry.h"
#include "candidates/FirstBisectionalChoice.h"
#include "candidates/CompetingCandidates.h"
#include "dispatcher/InstantDirectMessage.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator)
  : evaluator(evaluator)
{
  // Decisions come from Candidates collections, each owned by a generic GreedyDispatcher (templated on the
  // collection type), tried in priority order. Exit, entry, and direct message delivery are unambiguous;
  // the contested sequential-ad-hoc entries and message deliveries are merged into one reward-ordered
  // collection so their best feasible decision competes within the same list.
  dispatchers.push_back( std::make_unique<GreedyDispatcher<FirstFeasibleExit>>(evaluator) );
  dispatchers.push_back( std::make_unique<GreedyDispatcher<FirstFeasibleEntry>>(evaluator) ); // non-sequential entries only (config.sequential=false)
  dispatchers.push_back( std::make_unique<InstantDirectMessage>() );
  dispatchers.push_back( std::make_unique<GreedyDispatcher<FirstBisectionalChoice>>(evaluator) );
  dispatchers.push_back( std::make_unique<GreedyDispatcher<CompetingCandidates>>(evaluator) ); // sequential ad-hoc entries and message deliveries
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : dispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GreedyController::dispatchEvent(const SystemState* systemState) {
  // Dispatch the first feasible decision in priority order; forward any non-decision event immediately.
  for ( auto& eventDispatcher : dispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      if ( auto decision = dynamic_pointer_cast<Decision>(event) ) {
        if ( decision->reward().has_value() ) {
          return event;
        }
      }
      else {
        // events are immediately forwarded
        return event;
      }
    }
  }

  return nullptr;
}
