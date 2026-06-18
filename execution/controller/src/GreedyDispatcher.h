#ifndef BPMNOS_Execution_GreedyDispatcher_H
#define BPMNOS_Execution_GreedyDispatcher_H

#include <bpmn++.h>
#include <memory>
#include <tuple>
#include <limits>
#include <utility>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

class Mediator;

/**
 * @brief Generic greedy policy dispatcher owning a concrete Candidates collection.
 *
 * Templated on the Candidates type (a Candidates specialization), which it owns by value and constructs by
 * forwarding the constructor arguments. On each dispatch it iterates the reward-ordered candidates for the
 * current state (iterating them evaluates them lazily) and dispatches the best feasible one — the front of
 * the set, unless that front is infeasible.
 */
template <typename Candidates>
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
