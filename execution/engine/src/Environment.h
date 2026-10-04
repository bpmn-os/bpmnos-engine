#ifndef BPMNOS_Execution_Environment_H
#define BPMNOS_Execution_Environment_H

#include "EventDispatcher.h"
#include "Observer.h"
#include "execution/data/src/DataProvider.h"
#include "execution/data/src/Scenario.h"

namespace BPMNOS::Execution {

/**
 * @brief Class connecting a run to the scenario it executes.
 *
 * The environment observes the engine and forwards every notification it receives, including the
 * installation of a system state, to the data provider of the scenario of the run, together with the
 * scenario and its queue of enqueued events. It forwards every request of the engine for an event as
 * well, and then answers it with the first enqueued event. The queue is first in, first out, and the
 * environment never reorders it, so the events are dispatched in the order in which the data provider
 * enqueues them. It owns the scenario of the run, which the engine hands to it whenever a run is
 * initialised or a system state is installed.
 */
class Environment : public EventDispatcher, public Observer {
public:
  void connect(Mediator* mediator) override;
  using EventDispatcher::notice;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;

  /**
   * @brief Method taking ownership of the scenario of the run, which must be set before the system state of
   * the run is announced.
   */
  void setScenario(std::unique_ptr<Scenario> scenario);

private:
  std::unique_ptr<Scenario> scenario; ///< The scenario of the run
  EventQueue enqueuedEvents; ///< Events enqueued but not yet dispatched, first in, first out
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Environment_H
