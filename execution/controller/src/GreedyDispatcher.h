#ifndef BPMNOS_Execution_GreedyDispatcher_H
#define BPMNOS_Execution_GreedyDispatcher_H

#include <bpmn++.h>
#include <memory>
#include <tuple>
#include <limits>
#include <utility>
#include <concepts>
#include <type_traits>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

class Mediator;

/// A candidate element: a tuple-like value whose first component is the reward and whose second-to-last
/// component is the decision's weak Event — the only parts a dispatcher reads.
template <typename T>
concept CandidateElement = requires (T candidate) {
  { std::get<0>(candidate) } -> std::convertible_to<double>;
  { std::get<std::tuple_size_v<T> - 2>(candidate) } -> std::convertible_to<std::weak_ptr<Event>>;
};

/// A reward-ordered candidate collection a dispatcher can drive: connectable to a mediator and iterable over
/// CandidateElements (best first).
template <typename C>
concept CandidateCollection = requires (C& collection, Mediator* mediator) {
  collection.connect(mediator);
  collection.begin() != collection.end();
  ++std::declval< decltype(collection.begin())& >();
  requires CandidateElement< std::remove_cvref_t<decltype(*collection.begin())> >;
};

/**
 * @brief Generic greedy policy dispatcher owning a concrete Candidates collection.
 *
 * Templated on the Candidates type (any CandidateCollection), which it owns by value and constructs by
 * forwarding the constructor arguments. On each dispatch it iterates the reward-ordered candidates for the
 * current state (iterating them evaluates them lazily) and dispatches the best feasible one — the front of
 * the set, unless that front is infeasible.
 */
template <CandidateCollection Candidates>
class GreedyDispatcher : public EventDispatcher {
public:
  template <typename... Args>
  GreedyDispatcher(Args&&... args) : candidates(std::forward<Args>(args)...) {}

  std::shared_ptr<Event> dispatchEvent( [[maybe_unused]] const SystemState* systemState ) override {
    for ( auto candidate : candidates ) {
      constexpr std::size_t eventIndex = std::tuple_size<decltype(candidate)>::value - 2;
      std::weak_ptr<Event>& event_ptr = std::get<eventIndex>(candidate);
      if ( auto event = event_ptr.lock();
        event && std::get<0>(candidate) > -std::numeric_limits<double>::infinity()
      ) {
        // dispatch the best feasible decision
        return event;
      }
      else {
        // best decision is infeasible, no need to inspect others
        break;
      }
    }
    return nullptr;
  }

  void connect(Mediator* mediator) override {
    candidates.connect(mediator);   // the candidates subscribe to their request type and DataUpdate
    EventDispatcher::connect(mediator);
  }

protected:
  Candidates candidates;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyDispatcher_H
