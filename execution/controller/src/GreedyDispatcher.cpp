#include "GreedyDispatcher.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

template <typename... WeakPtrs>
GreedyDispatcher<WeakPtrs...>::GreedyDispatcher(Evaluator* evaluator)
  : evaluator(evaluator)
{
  if ( !evaluator ) {
    throw std::runtime_error("GreedyDispatcher: missing evaluator");
  }
  timestamp = std::numeric_limits<BPMNOS::number>::lowest();
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Observable::Type::DataUpdate
  );
  EventDispatcher::connect(mediator);
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::addEvaluation(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision, std::optional<double> reward) {
//std::cerr << reward.value_or(-999) << std::endl;  
  // evaluatedDecisions are sorted in ascending order
  // decisions without reward are assumed to be infeasible
  evaluatedDecisions.emplace( (reward.has_value() ? -(double)reward.value() : std::numeric_limits<double>::max() ), weak_ptrs..., decision->weak_from_this());

//std::cerr << "GreedyDispatcher: evaluated decision: " <<  decision->jsonify() << " with " << reward.value_or(-999) << std::endl; 
  assert ( decision );

  // add evaluation to respective container
  if ( !decision->timeDependent && decision->dataDependencies.empty() ) {
//std::cerr << "GreedyDispatcher: Decision is invariant"<< std::endl;
    invariantEvaluations.emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.empty() ) {
//std::cerr << "GreedyDispatcher: Decision is time dependent" << std::endl;
    timeDependentEvaluations.emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( !decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
//std::cerr << "GreedyDispatcher: Decision is data dependent for instance  " << BPMNOS::to_string(instanceId,STRING) << std::endl;
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
//std::cerr << "GreedyDispatcher: Decision is time and data dependent for instance " <<  BPMNOS::to_string(instanceId,STRING) << std::endl;
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
}

template <typename... WeakPtrs>
std::shared_ptr<Event> GreedyDispatcher<WeakPtrs...>::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for ( auto& decisionTuple : decisionsWithoutEvaluation ) {
    std::apply([this,&decisionTuple](auto&&... args) {
      auto decision = std::get<std::shared_ptr<Decision>>(decisionTuple);
      assert(decision);
      // Call decision->evaluate() and add the evaluation
      auto reward = decision->evaluate();
      this->addEvaluation(std::forward<decltype(args)>(args)..., reward);
    }, decisionTuple);
  }
  decisionsWithoutEvaluation.clear();

  for ( auto decisionTuple : evaluatedDecisions ) {
    std::weak_ptr<Event>& event_ptr = std::get<sizeof...(WeakPtrs)+1>(decisionTuple);
//std::cerr << "\nBest decision " << event_ptr.lock()->jsonify() << " evaluated with " << std::get<0>(decisionTuple) << std::endl;
    return event_ptr.lock();
  }

  return nullptr;
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::notice(const Observable* observable) {
//std::cerr << "GreedyDispatcher:noticed event" << std::endl;
  if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
    dataUpdate(static_cast<const DataUpdate*>(observable));   
  }
}

template <typename... WeakPtrs>
bool GreedyDispatcher<WeakPtrs...>::intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const {
  for ( auto lhs : first ) {
    if ( second.contains(lhs) ) {
      return true;
    }
  }
  return false;
};

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions) {
  // check whether evaluation has become obsolete
  for ( auto it = evaluation.begin(); it != evaluation.end(); ) {
    auto& decisionTuple = *it;
    std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
    if ( intersect(update->attributes, decision->dataDependencies) ) {
      decision->reward = std::nullopt;
      std::apply([&unevaluatedDecisions](auto&&... args) { unevaluatedDecisions.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
      // remove evaluation
//std::cerr << "GreedyDispatcher: Remove evaluation of decision: " <<  decision->jsonify() << std::endl; 
      it = evaluation.erase(it);
    }
    else {
      ++it;
    }
  }
};

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions)  {
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
void GreedyDispatcher<WeakPtrs...>::dataUpdate(const DataUpdate* update) {
  removeDependentEvaluations(update,dataDependentEvaluations,decisionsWithoutEvaluation);
  removeDependentEvaluations(update,timeAndDataDependentEvaluations,decisionsWithoutEvaluation);
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::clockTick() {
  for ( auto& decisionTuple : timeDependentEvaluations ) {
    std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
    decision->reward = std::nullopt;
    std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
  }

  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& decisionTuple : evaluations ) {
      std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
      decision->reward = std::nullopt;
      std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
    }
  }
  timeAndDataDependentEvaluations.clear();
}

template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;



