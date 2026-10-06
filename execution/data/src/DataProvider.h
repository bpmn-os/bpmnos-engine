#ifndef BPMNOS_Execution_DataProvider_H
#define BPMNOS_Execution_DataProvider_H

#include <deque>
#include <memory>
#include "model/bpmnos/src/Model.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

struct Observable;
struct Event;
class SystemState;
class Scenario;

typedef std::deque< std::shared_ptr<Event> > EventQueue; ///< Events enqueued for the engine, first in, first out

/**
 * @brief Base class of the data providers feeding the environment of a run with events.
 *
 * A data provider holds no state of a run: everything it records about a run is held by the scenario of
 * the run, which it is given with every call, so that one data provider serves every scenario it creates.
 * Through the environment it is notified of everything the engine announces, asked for the events due at
 * the current time, and asked to advance, and it enqueues the events the environment then dispatches in
 * the order in which they are enqueued.
 *
 * A data provider decides whether time advances. When it is asked to advance, neither it nor the
 * controller having supplied an event at the current time, the base enqueues the next clock tick. A data
 * provider that has to keep time regardless of the controller, such as one observing the real world,
 * enqueues its clock ticks when asked for the events due at the current time instead. With a clock
 * tick duration of zero it does so at once; otherwise it does so once the wall clock has reached the given
 * number of milliseconds after the previous clock tick, and supplies nothing before. A data provider never
 * blocks, since the controller may decide while time does not advance.
 */
class DataProvider : public std::enable_shared_from_this<DataProvider> {
public:
  /**
   * @param model The model the data provider is built on.
   * @param clockTickDuration Milliseconds of wall-clock time between two clock ticks, zero meaning none.
   */
  DataProvider(const BPMNOS::Model::Model* model, unsigned int clockTickDuration = 0);
  virtual ~DataProvider() = default;

  /**
   * @brief Method returning the model the data provider is built on.
   */
  const BPMNOS::Model::Model* getModel() const;

  /**
   * @brief Method returning the values of the global attributes at the beginning of a run on the scenario.
   */
  virtual BPMNOS::Values getGlobals(const Scenario& scenario) const = 0;

  /**
   * @brief Method returning the earliest time at which an instance of the scenario is instantiated.
   */
  virtual BPMNOS::number getEarliestInstantiationTime(const Scenario& scenario) const = 0;

  const unsigned int clockTickDuration; ///< Milliseconds of wall-clock time between two clock ticks

  /**
   * @brief Method notifying the data provider of a notification of the engine, including the
   * installation of a system state.
   */
  virtual void notice(const Observable* observable, Scenario& scenario, EventQueue& queue) const = 0;

  /**
   * @brief Method enqueuing the events due at the current time, called at the beginning of every round of
   * the engine.
   */
  virtual void dispatchEvent(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const = 0;

  /**
   * @brief Method letting the data provider advance, if it has not done so already, called when neither
   * the data provider nor the controller has supplied an event at the current time.
   *
   * The base enqueues the next clock tick once it is due.
   */
  virtual void advance(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const;

private:
  const BPMNOS::Model::Model* model; ///< The model the data provider is built on
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataProvider_H
