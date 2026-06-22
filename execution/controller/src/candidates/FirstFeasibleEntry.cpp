#include "FirstFeasibleEntry.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstFeasibleEntry::FirstFeasibleEntry(Evaluator* evaluator, Config config)
  : evaluator(evaluator)
  , config(config)
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
    if ( !config.sequential ) {
      assert( request->token->node->parent );
      if ( request->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        // entries of sequential ad-hoc subprocess children are left to a competing source
        return;
      }
    }
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
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
