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
  decisionStore.advanceTime(systemState->currentTime);

  decisionStore.evaluateDecisions([this](WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) -> std::shared_ptr<Event> {
    assert(decision);
    // Call decision->evaluate() and add the evaluation
    decision->evaluate();
    decisionStore.addEvaluation(weak_ptrs..., std::move(decision));
    return nullptr;
  });

  return decisionStore.getBestDecision();
}

template <typename... WeakPtrs>
void GreedyDispatcher<WeakPtrs...>::notice(const Observable* observable) {
//std::cerr << "GreedyDispatcher:noticed event" << std::endl;
  decisionStore.notice(observable);
}

template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >;
template class BPMNOS::Execution::GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> >;
