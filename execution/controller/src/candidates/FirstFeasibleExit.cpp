#include "FirstFeasibleExit.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstFeasibleExit::FirstFeasibleExit(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

void FirstFeasibleExit::connect(Notifier* notifier) {
  notifier->addSubscriber(this,
    Observable::Type::ExitRequest,
    Observable::Type::DataUpdate,
    Observable::Type::SystemState
  );
}

void FirstFeasibleExit::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::ExitRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    auto decision = std::make_shared<ExitDecision>(request, evaluator);
    addDecision( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else if ( observable->getObservableType() == Observable::Type::SystemState ) {
    clear();   // start from a clean cache, then rebuild from the installed state
    systemState = static_cast<const SystemState*>(observable);
    // rebuild the cache from the exit decisions a freshly installed (e.g. resumed) state lists as pending
    for ( auto& [_, request_ptr] : systemState->pendingExitDecisions ) {
      if ( auto request = request_ptr.lock() ) {
        notice( request.get() );
      }
    }
  }
  else {
    CachedCandidates::notice(observable);
  }
}

void FirstFeasibleExit::evaluateCandidates() {
  advanceTime(systemState->currentTime);
  evaluateDecisions(
    [this]( std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision ) -> std::shared_ptr<Event> {
      decision->evaluate();
      addEvaluation(token_ptr, request_ptr, decision);
      if ( decision->reward().has_value() ) {
        return decision;   // stop at the first feasible candidate
      }
      return nullptr;
    }
  );
}
