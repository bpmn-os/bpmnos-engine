#include "GreedyController.h"
#include "execution/engine/src/Engine.h"
#include "GreedyDispatcher.h"
#include "candidates/FirstChoice.h"
#include "candidates/CompetingCandidates.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(std::shared_ptr<Evaluator> evaluator, Config config)
  : config(config)
{
  // Decisions come from Candidates collections, each owned by a generic GreedyDispatcher (templated on the
  // collection type), tried in priority order. The engine itself enters the activities whose entry is not
  // decided and delivers the directly addressed messages; choices, which compete with nothing, are made first,
  // and the contested sequential-ad-hoc entries and message deliveries are merged into one reward-ordered
  // collection so their best feasible decision competes within the same list.
  dispatchers.push_back( std::make_unique<GreedyDispatcher<FirstChoice>>(evaluator, config.bisect) );
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
