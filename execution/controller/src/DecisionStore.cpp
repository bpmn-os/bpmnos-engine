#include "DecisionStore.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

template <typename... WeakPtrs>
DecisionStore<WeakPtrs...>::DecisionStore()
{
  timestamp = std::numeric_limits<BPMNOS::number>::lowest();
}

template <typename... WeakPtrs>
void DecisionStore<WeakPtrs...>::addEvaluation(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
  auto reward = decision->reward();
  // evaluatedDecisions are sorted in ascending order
  // decisions without reward are assumed to be infeasible
  evaluatedDecisions.emplace( (reward.has_value() ? -(double)reward.value() : std::numeric_limits<double>::max() ), weak_ptrs..., decision->weak_from_this(), decision->evaluation);

  assert ( decision );

  // add evaluation to respective container
  if ( !decision->timeDependent && decision->dataDependencies.empty() ) {
    invariantEvaluations.emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.empty() ) {
    timeDependentEvaluations.emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( !decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
}

template <typename... WeakPtrs>
bool DecisionStore<WeakPtrs...>::intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const {
  for ( auto lhs : first ) {
    if ( second.contains(lhs) ) {
      return true;
    }
  }
  return false;
};

template <typename... WeakPtrs>
void DecisionStore<WeakPtrs...>::removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions) {
  // check whether evaluation has become obsolete
  for ( auto it = evaluation.begin(); it != evaluation.end(); ) {
    auto& decisionTuple = *it;
    std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
    if ( intersect(update->attributes, decision->dataDependencies) ) {
      decision->evaluation.reset();
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
void DecisionStore<WeakPtrs...>::removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions)  {
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
void DecisionStore<WeakPtrs...>::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
    dataUpdate(static_cast<const DataUpdate*>(observable));
  }
}

template <typename... WeakPtrs>
void DecisionStore<WeakPtrs...>::dataUpdate(const DataUpdate* update) {
  removeDependentEvaluations(update,dataDependentEvaluations,decisionsWithoutEvaluation);
  removeDependentEvaluations(update,timeAndDataDependentEvaluations,decisionsWithoutEvaluation);
}

template <typename... WeakPtrs>
void DecisionStore<WeakPtrs...>::clockTick() {
  for ( auto& decisionTuple : timeDependentEvaluations ) {
    std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
    decision->evaluation.reset();
    std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
  }

  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& decisionTuple : evaluations ) {
      std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
      decision->evaluation.reset();
      std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
    }
  }
  timeAndDataDependentEvaluations.clear();
}

template class BPMNOS::Execution::DecisionStore< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::DecisionStore< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;
