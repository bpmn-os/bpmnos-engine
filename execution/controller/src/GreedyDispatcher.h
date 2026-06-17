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
 * @brief Generic greedy policy dispatcher owning a concrete Candidates source.
 *
 * Templated on the source type (a Candidates specialization), which it owns by value and constructs by
 * forwarding the constructor arguments. On each dispatch it obtains the reward-ordered candidates for the
 * current state (Source::getCandidates populates them via evaluateCandidates) and dispatches the best
 * feasible one — the front of the set, unless that front is infeasible.
 */
template <typename Source>
class GreedyDispatcher : public EventDispatcher {
public:
  template <typename... Args>
  GreedyDispatcher(Args&&... args) : source(std::forward<Args>(args)...) {}

  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override {
    for ( auto candidate : source.getCandidates(systemState) ) {
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
    source.connect(mediator);   // the source subscribes to its request type and DataUpdate
    EventDispatcher::connect(mediator);
  }

protected:
  Source source;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyDispatcher_H
