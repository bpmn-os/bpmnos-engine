#include "FirstFeasibleEntry.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstFeasibleEntry::FirstFeasibleEntry(std::shared_ptr<Evaluator> evaluator)
  : CachedCandidates(std::move(evaluator))
{
}

void FirstFeasibleEntry::connect(Notifier* notifier) {
  notifier->addSubscriber(this,
    Observable::Type::EntryRequest,
    Observable::Type::DataUpdate,
    Observable::Type::SystemState
  );
}

void FirstFeasibleEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    // the engine raises entry requests only for children of sequential ad-hoc subprocesses
    assert( BPMNOS::Model::Model::isSequentialActivity(request->token->node) );
    auto decision = std::make_shared<EntryDecision>(request, evaluator.get());
    addDecision( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else if ( observable->getObservableType() == Observable::Type::SystemState ) {
    clear();   // start from a clean cache, then rebuild from the installed state
    systemState = static_cast<const SystemState*>(observable);
    // rebuild the cache from the entry decisions a freshly installed (e.g. resumed) state lists as pending
    for ( auto& [_, request_ptr] : systemState->pendingEntryDecisions ) {
      if ( auto request = request_ptr.lock() ) {
        notice( request.get() );
      }
    }
  }
  else {
    CachedCandidates::notice(observable);
  }
}

void FirstFeasibleEntry::evaluateCandidates() {
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
