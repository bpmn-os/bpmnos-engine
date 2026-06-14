#include "GreedyController.h"
#include "dispatcher/greedy/GreedyEntry.h"
#include "dispatcher/greedy/GreedyExit.h"
#include "dispatcher/greedy/BestFirstEntry.h"
#include "dispatcher/greedy/BisectionalChoice.h"
#include "dispatcher/greedy/BestMatchingMessageDelivery.h"
#include "dispatcher/naive/InstantDirectMessage.h"
#include <iostream>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(Evaluator* evaluator)
  : evaluator(evaluator)
{
  // Prioritized layer: dispatch the first feasible decision.
  prioritizedDispatchers.push_back( std::make_unique<GreedyExit>(evaluator) );
  prioritizedDispatchers.push_back( std::make_unique<GreedyEntry>(evaluator, GreedyEntry::Config{ .sequential = false }) ); // non-sequential entries only
  prioritizedDispatchers.push_back( std::make_unique<InstantDirectMessage>() );
  prioritizedDispatchers.push_back( std::make_unique<BisectionalChoice>(evaluator, BisectionalChoice::Config{ .firstFeasible = true }) );
  // Competing layer: best-of-best over the contested decisions.
  competingDispatchers.push_back( std::make_unique<BestFirstEntry>(evaluator, BestFirstEntry::Config{ .onlySequential = true }) ); // sequential ad-hoc entries only
  competingDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(evaluator) );
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
