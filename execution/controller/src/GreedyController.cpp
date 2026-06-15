#include "GreedyController.h"
#include "execution/engine/src/Engine.h"
#include "GreedyCandidateDispatcher.h"
#include "candidates/FirstFeasibleExit.h"
#include "candidates/FirstFeasibleEntry.h"
#include "candidates/SequentialEntries.h"
#include "candidates/FirstBisectionalChoice.h"
#include "candidates/MessageDeliveries.h"
#include "dispatcher/naive/InstantDirectMessage.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator)
  : evaluator(evaluator)
{
  // Entry, exit, sequential, and message-delivery decisions come from Candidates sources, each owned by a
  // generic GreedyCandidateDispatcher (templated on the source type).
  // Prioritized layer: dispatch the first feasible decision.
  prioritizedDispatchers.push_back( std::make_unique<GreedyCandidateDispatcher<FirstFeasibleExit>>(evaluator) );
  prioritizedDispatchers.push_back( std::make_unique<GreedyCandidateDispatcher<FirstFeasibleEntry>>(evaluator) ); // non-sequential entries only (config.sequential=false)
  prioritizedDispatchers.push_back( std::make_unique<InstantDirectMessage>() );
  prioritizedDispatchers.push_back( std::make_unique<GreedyCandidateDispatcher<FirstBisectionalChoice>>(evaluator) );
  // Competing layer: best-of-best over the contested decisions.
  competingDispatchers.push_back( std::make_unique<GreedyCandidateDispatcher<SequentialEntries>>(evaluator) ); // sequential ad-hoc entries only
  competingDispatchers.push_back( std::make_unique<GreedyCandidateDispatcher<MessageDeliveries>>(evaluator) );
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : prioritizedDispatchers ) {
    eventDispatcher->connect(this);
  }
  for ( auto& eventDispatcher : competingDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GreedyController::dispatchEvent(const SystemState* systemState) {
  // Instant layer: dispatch the first feasible decision in priority order.
  for ( auto& eventDispatcher : prioritizedDispatchers ) {
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

  // Competing layer: dispatch the best evaluated decision (best-of-best).
  std::shared_ptr<Decision> best = nullptr;
  for ( auto& eventDispatcher : competingDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      if (  auto decision = dynamic_pointer_cast<Decision>(event) ) {
        if ( decision->reward().has_value() ) {
          if ( !best ) {
            // first feasible decision is used as best
            best = decision;
          }
          else if ( decision->reward().value() > best->reward().value() ) {
            // decision has better reward than current best
            best = decision;
          }
        }
      }
      else {
        // events are immediately forwarded
        return event;
      }
    }
  }

  return best;
}
