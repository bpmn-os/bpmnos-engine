#ifndef BPMNOS_Execution_Environment_H
#define BPMNOS_Execution_Environment_H

#include "Event.h"
#include "SystemState.h"
#include "Observer.h"
#include "execution/data/src/DataProvider.h"
#include "execution/data/src/Scenario.h"

namespace BPMNOS::Execution {

struct Mediator;

/**
 * @brief Class connecting a run to the scenario it executes.
 *
 * The environment observes the engine and forwards every notification it receives, including the
 * installation of a system state, to the data provider of the scenario of the run, together with the
 * scenario and its queue of enqueued events. In every round the engine first asks it to dispatch an event
 * due at the current time, and, if neither it nor the controller supplies one, asks it to advance. It
 * forwards both to the data provider as well, and answers each with the first enqueued event. The queue
 * is first in, first out, and the environment never reorders it, so the events are dispatched in the order
 * in which the data provider enqueues them; an event that has expired while it was enqueued is discarded. It owns the scenario of the run, which the engine hands to it whenever a run is
 * initialised or a system state is installed.
 */
class Environment : public Observer {
public:
  void connect(Mediator* mediator);
  void notice(const Observable* observable) override;

  /**
   * @brief Method letting the data provider enqueue the events due at the current time and returning the
   * first enqueued event, if any.
   */
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);

  /**
   * @brief Method letting the data provider advance, if it has not done so already, called when nothing is
   * left to do at the current time, and returning the first enqueued event, if any.
   */
  std::shared_ptr<Event> advance(const SystemState* systemState);

  /**
   * @brief Method taking ownership of the scenario of the run, which must be set before the system state of
   * the run is announced.
   */
  void setScenario(std::unique_ptr<Scenario> scenario);

private:
  std::unique_ptr<Scenario> scenario; ///< The scenario of the run

  std::shared_ptr<Event> dequeueEvent(); ///< Removes and returns the first enqueued event, if any
  EventQueue enqueuedEvents; ///< Events enqueued but not yet dispatched, first in, first out
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Environment_H
