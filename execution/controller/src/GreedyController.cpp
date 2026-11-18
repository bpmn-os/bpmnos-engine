#include "GreedyController.h"
#include "execution/engine/src/Engine.h"
#include "GreedyDispatcher.h"
#include "candidates/Choices.h"
#include "candidates/SequentialEntries.h"
#include "candidates/MessageDeliveries.h"
#include <iostream>
#include <limits>

using namespace BPMNOS::Execution;

GreedyController::GreedyController(std::shared_ptr<Evaluator> evaluator, Config config)
  : config(config)
{
  // Decisions come from Candidates collections, each owned by a generic GreedyDispatcher (templated on the
  // collection type). The engine itself enters the activities whose entry is not decided and delivers the directly
  // addressed messages; the choices, the sequential-ad-hoc entries and the message deliveries compete, the best
  // feasible decision of each collection being compared in dispatchEvent.
  dispatchers.push_back( std::make_unique<GreedyDispatcher<Choices>>(evaluator, config.bisect) );
  dispatchers.push_back( std::make_unique<GreedyDispatcher<SequentialEntries>>(evaluator) );
  dispatchers.push_back( std::make_unique<GreedyDispatcher<MessageDeliveries>>(evaluator) );
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : dispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

std::shared_ptr<Event> GreedyController::dispatchEvent(const SystemState* systemState) {
  // Dispatch the decision of the highest reward among the best feasible decisions of the dispatchers, a tie going to
  // the dispatcher listed first; forward any non-decision event immediately.
  std::shared_ptr<Event> best;
  double bestReward = -std::numeric_limits<double>::infinity();
  for ( auto& eventDispatcher : dispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      if ( auto decision = dynamic_pointer_cast<Decision>(event) ) {
        if ( auto reward = decision->reward(); reward.has_value() && ( !best || reward.value() > bestReward ) ) {
          best = event;
          bestReward = reward.value();
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
