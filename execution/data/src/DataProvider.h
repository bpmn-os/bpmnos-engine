#ifndef BPMNOS_Execution_DataProvider_H
#define BPMNOS_Execution_DataProvider_H

#include <deque>
#include <memory>

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
 * Through the environment it is notified of everything the engine announces and of every request of the
 * engine for an event, and it enqueues the events the environment then dispatches in the order in which
 * they are enqueued.
 */
class DataProvider : public std::enable_shared_from_this<DataProvider> {
public:
  virtual ~DataProvider() = default;

  /**
   * @brief Method notifying the data provider of a notification of the engine, including the
   * installation of a system state.
   */
  virtual void notice(const Observable* observable, Scenario& scenario, EventQueue& queue) const = 0;

  /**
   * @brief Method notifying the data provider of a request of the engine for an event, before the
   * request is answered with the first enqueued event.
   */
  virtual void request(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataProvider_H
