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
void GreedyDispatcher<WeakPtrs...>::evaluate(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
  assert ( decision );

  auto value = decision->evaluate();
  // decisions without evaluation are assumed to be infeasible
  evaluatedDecisions.emplace( (value.has_value() ? (double)value.value() : std::numeric_limits<double>::max() ), weak_ptrs..., decision->weak_from_this());

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
//std::cerr << "GreedyDispatcher: Decision is data dependent with index " << (long unsigned int)instanceId << std::endl;
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
//std::cerr << "GreedyDispatcher: Decision is time and data dependent with index " << (long unsigned int)instanceId << std::endl;
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(weak_ptrs..., std::move(decision) );
  }
}

template <typename... WeakPtrs>
std::shared_ptr<Event> GreedyDispatcher<WeakPtrs...>::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for ( auto& decisionTuple : decisionsWithoutEvaluation ) {
    std::apply([this](auto&&... args) { this->evaluate(std::forward<decltype(args)>(args)...); }, decisionTuple);
  }
  decisionsWithoutEvaluation.clear();

  for ( auto decisionTuple : evaluatedDecisions ) {
    std::weak_ptr<Event>& event_ptr = std::get<sizeof...(WeakPtrs)+1>(decisionTuple);
    return event_ptr.lock();
  }

  return nullptr;
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
    dataUpdate(static_cast<const DataUpdate*>(observable));   
  }
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::dataUpdate(const DataUpdate* update) {
/*
std::cerr << "DataUpdate: ";
for ( auto attribute : update->attributes ) {
std::cerr << attribute->name << ", ";
}
std::cerr << std::endl;
*/
  auto removeDependentEvaluations = [this,&update](std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluations) -> void {
    auto intersect = [](const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) -> bool {
      for ( auto lhs : first ) {
        if ( second.contains(lhs) ) {
          return true;
        }
      }
      return false;
    };

    auto removeObsolete = [this,&update,intersect](auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation) -> void {
      // check whether evaluation has become obsolete
      for ( auto it = evaluation.begin(); it != evaluation.end(); ) {
        auto& decisionTuple = *it;
        std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
        if ( intersect(update->attributes, decision->dataDependencies) ) {
          decision->evaluation = std::nullopt;
          std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
          // remove evaluation
          it = evaluation.erase(it);
        }
        else {
          ++it;
        }
      }
    };

    if ( update->instanceId >= 0 ) {
      // find instance that data update refers to
      if ( auto it = evaluations.find((long unsigned int)update->instanceId);
        it != evaluations.end()
      ) {
        removeObsolete(it->second);
      }
    }
    else {
      // update of global value may influence evaluations of all instances
      for ( auto it = evaluations.begin(); it != evaluations.end(); ++it) {
        removeObsolete(it->second);
      }
    }
  };
    
  removeDependentEvaluations(dataDependentEvaluations);
  removeDependentEvaluations(timeAndDataDependentEvaluations);
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::clockTick() {
  for ( auto& decisionTuple : timeDependentEvaluations ) {
    std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
    decision->evaluation = std::nullopt;
    std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
  }

  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& decisionTuple : evaluations ) {
      std::shared_ptr<Decision>& decision = std::get<sizeof...(WeakPtrs)>(decisionTuple);
      decision->evaluation = std::nullopt;
      std::apply([this](auto&&... args) { this->decisionsWithoutEvaluation.emplace_back(std::forward<decltype(args)>(args)...); }, decisionTuple);
    }
  }
  timeAndDataDependentEvaluations.clear();
}

template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;



