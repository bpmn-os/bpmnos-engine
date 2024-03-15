#include "GreedyController.h"
#include "dispatcher/naive/InstantEntry.h"
#include "dispatcher/greedy/BestFirstSequentialEntry.h"
#include "dispatcher/naive/RandomChoice.h"
#include "dispatcher/naive/FirstMatchingMessageDelivery.h"
#include "dispatcher/naive/InstantExit.h"

using namespace BPMNOS::Execution;

GreedyController::GreedyController()
{
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<InstantEntry>() );
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>() );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() );
  eventDispatchers.push_back( std::make_unique<FirstMatchingMessageDelivery>() );
  eventDispatchers.push_back( std::make_unique<InstantExit>() );
//  eventDispatchers.push_back( std::make_unique<MyopicMessageTaskTerminator>() );
//  eventDispatchers.push_back( std::make_unique<MyopicDecisionTaskTerminator>() );
//  eventDispatchers.push_back( std::make_unique<TimeWarp>() );
}

void GreedyController::connect(Mediator* mediator) {
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
  Controller::connect(mediator);
}

