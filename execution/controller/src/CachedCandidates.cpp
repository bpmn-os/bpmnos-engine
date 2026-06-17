#include "CachedCandidates.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

template <typename... WeakPtrs>
CachedCandidates<WeakPtrs...>::CachedCandidates()
{
  timestamp = std::numeric_limits<BPMNOS::number>::lowest();
}

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::addDecision(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
  candidatesWithoutEvaluations.emplace_back(weak_ptrs..., decision); // weak index of the pending decision
  Candidates<WeakPtrs...>::addDecision(weak_ptrs..., std::move(decision)); // base takes ownership (and prunes the expired front)
}

template <typename... WeakPtrs>
template <std::size_t... I>
std::shared_ptr<Event> CachedCandidates<WeakPtrs...>::applyEvaluate(const Evaluate& evaluate, const std::tuple< WeakPtrs..., std::weak_ptr<Decision> >& tuple, std::shared_ptr<Decision> decision, std::index_sequence<I...>) {
  return evaluate(std::get<I>(tuple)..., std::move(decision));
}

template <typename... WeakPtrs>
std::shared_ptr<Event> CachedCandidates<WeakPtrs...>::evaluateDecisions(const Evaluate& evaluate) {
  for ( auto it = candidatesWithoutEvaluations.begin(); it != candidatesWithoutEvaluations.end(); ) {
    auto decisionTuple = *it; // copy the weak identifiers before erasing
    it = candidatesWithoutEvaluations.erase(it);
    auto decision = std::get<sizeof...(WeakPtrs)>(decisionTuple).lock();
    if ( !decision ) {
      continue; // decision already released
    }
    // immediately return decision provided by callback
    if ( auto event = applyEvaluate(evaluate, decisionTuple, std::move(decision), std::make_index_sequence<sizeof...(WeakPtrs)>{}) ) {
      return event;
    }
  }
  return nullptr;
}

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::addEvaluation(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
  assert ( decision );

  // candidates are sorted in descending order of reward; decisions without reward are assumed infeasible (-infinity, sorting last).
  // The decision is already owned in the base `decisions`, so this only lists the (weak) candidate.
  auto reward = decision->reward();
  this->candidates.emplace( (reward.has_value() ? (double)reward.value() : -std::numeric_limits<double>::infinity() ), weak_ptrs..., decision->weak_from_this(), decision->evaluation);

  // weak-index the evaluation for invalidation; invariant decisions never invalidate, so they need no index
  if ( decision->timeDependent && decision->dataDependencies.empty() ) {
    timeDependentEvaluations.emplace_back(weak_ptrs..., decision );
  }
  else if ( !decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., decision );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., decision );
  }
}

template <typename... WeakPtrs>
bool CachedCandidates<WeakPtrs...>::intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const {
  for ( auto lhs : first ) {
    if ( second.contains(lhs) ) {
      return true;
    }
  }
  return false;
};

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& unevaluatedDecisions) {
  // check whether evaluation has become obsolete
  for ( auto it = evaluation.begin(); it != evaluation.end(); ) {
    auto& decisionTuple = *it;
    auto decision = std::get<sizeof...(WeakPtrs)>(decisionTuple).lock();
    if ( decision && intersect(update->attributes, decision->dataDependencies) ) {
      decision->evaluation.reset(); // drops the candidate; re-queues the decision for re-evaluation
      std::apply([&unevaluatedDecisions](auto&&... args) { unevaluatedDecisions.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
      // remove evaluation
      it = evaluation.erase(it);
    }
    else {
      ++it;
    }
  }
};

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::weak_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& unevaluatedDecisions)  {
    if ( update->instanceId >= 0 ) {
      // find instance that data update refers to
      if ( auto it = evaluatedDecisions.find((long unsigned int)update->instanceId);
        it != evaluatedDecisions.end()
      ) {
        removeObsolete(update,it->second,unevaluatedDecisions);
      }
    }
    else {
      // update of global value may influence evaluatedDecisions of all instances
      for ( auto it = evaluatedDecisions.begin(); it != evaluatedDecisions.end(); ++it) {
        removeObsolete(update,it->second,unevaluatedDecisions);
      }
    }
  };


template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
    dataUpdate(static_cast<const DataUpdate*>(observable));
  }
}

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::dataUpdate(const DataUpdate* update) {
  removeDependentEvaluations(update,dataDependentEvaluations,candidatesWithoutEvaluations);
  removeDependentEvaluations(update,timeAndDataDependentEvaluations,candidatesWithoutEvaluations);
}

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::advanceTime(BPMNOS::number currentTime) {
  if ( currentTime > timestamp ) {
    timestamp = currentTime;
    clockTick();
  }
}

template <typename... WeakPtrs>
void CachedCandidates<WeakPtrs...>::clockTick() {
  for ( auto& decisionTuple : timeDependentEvaluations ) {
    if ( auto decision = std::get<sizeof...(WeakPtrs)>(decisionTuple).lock() ) {
      decision->evaluation.reset(); // drops the candidate; re-queues the decision for re-evaluation
      std::apply([this](auto&&... args) { this->candidatesWithoutEvaluations.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
    }
  }

  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& decisionTuple : evaluations ) {
      if ( auto decision = std::get<sizeof...(WeakPtrs)>(decisionTuple).lock() ) {
        decision->evaluation.reset();
        std::apply([this](auto&&... args) { this->candidatesWithoutEvaluations.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
      }
    }
  }
  timeAndDataDependentEvaluations.clear();
}

template class BPMNOS::Execution::CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;
