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
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Observable::Type::DataUpdate
  );
  EventDispatcher::connect(mediator);
}

template <typename... WeakPtrs>
std::shared_ptr<Event> GreedyDispatcher<WeakPtrs...>::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  if ( systemState->currentTime > decisionStore.timestamp ) {
    decisionStore.timestamp = systemState->currentTime;
    decisionStore.clockTick();
  }

  for ( auto& decisionTuple : decisionStore.decisionsWithoutEvaluation ) {
    std::apply([this,&decisionTuple](auto&&... args) {
      auto decision = std::get<std::shared_ptr<Decision>>(decisionTuple);
      assert(decision);
      // Call decision->evaluate() and add the evaluation
      decision->evaluate();
      this->decisionStore.addEvaluation(std::forward<decltype(args)>(args)...);
    }, decisionTuple);
  }
  decisionStore.decisionsWithoutEvaluation.clear();

  for ( auto decisionTuple : decisionStore.evaluatedDecisions ) {
    std::weak_ptr<Event>& event_ptr = std::get<sizeof...(WeakPtrs)+1>(decisionTuple);
//std::cerr << "\nBest decision " << event_ptr.lock()->jsonify() << " evaluated with " << std::get<0>(decisionTuple) << std::endl;
    return event_ptr.lock();
  }

  return nullptr;
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::notice(const Observable* observable) {
//std::cerr << "GreedyDispatcher:noticed event" << std::endl;
  decisionStore.notice(observable);
}

template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;
