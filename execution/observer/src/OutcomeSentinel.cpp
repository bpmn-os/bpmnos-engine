#include "OutcomeSentinel.h"

using namespace BPMNOS::Execution;

void OutcomeSentinel::subscribe(Engine* engine) {
  engine->addSubscriber(this, Execution::Observable::Type::Token, Execution::Observable::Type::Event);
  runningInstances = 0;
}

void OutcomeSentinel::notice(const Observable* observable) {
  if ( firstObservation ) return;
  
  if ( observable->getObservableType() ==  Execution::Observable::Type::Token ) {
    auto token = static_cast<const Token*>(observable);
    if ( !token->node ) {
      // token for process
      if ( token->state == Token::State::ENTERED ) {
        runningInstances++;
      }
      else if ( token->state == Token::State::COMPLETED ) {
        runningInstances--;
      }
      else if ( token->state == Token::State::FAILED ) {
        firstObservation = Outcome::FAILED;
      }
    }
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Event ) {
    auto event = static_cast<const Event*>(observable);
    if ( event->is<TerminationEvent>() ) {
      firstObservation = Outcome::TERMINATED;
    }
  }
}


Outcome OutcomeSentinel::getOutcome() const {
  if ( firstObservation ) return firstObservation.value();
  if ( !runningInstances ) return Outcome::COMPLETED;
  return Outcome::INCOMPLETE;
}

