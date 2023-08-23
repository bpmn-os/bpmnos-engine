#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include <set>
#include <vector>
#include "model/data/src/DataProvider.h"
#include "StateMachine.h"

namespace BPMNOS::Execution {

class Engine {
public:
  Engine(BPMNOS::Model::DataProvider* dataProvider);
  BPMNOS::Model::DataProvider* dataProvider;

  void start(BPMNOS::number time = 0);
/**
 * @brief Returns the current timestamp of execution.
 */
  BPMNOS::number getTimestamp();

protected:
  struct AnticipatedInstantiation {
    BPMNOS::number anticipatedTime;
    const BPMNOS::Model::InstanceData* instance;
    bool operator<(const AnticipatedInstantiation& other) const {
        return anticipatedTime < other.anticipatedTime;
    }
  };

  BPMNOS::number timestamp;
/**
 * @brief Starts instances whose start time has been reached.
 *
 * This function iterates through the set of anticipated instances and
 * starts all instances for which the actual or assumed start time has
 * been reached.
 */
  void startInstances();

  void startInstance(const BPMNOS::Model::InstanceData* instance);

  std::vector< std::unique_ptr<StateMachine> > completedInstances; ///< Vector containing the final states of completed instances (even if failed).
  std::vector< std::unique_ptr<StateMachine> > runningInstances; ///< Vector containing the current state of running instances.
  std::set< AnticipatedInstantiation > anticipatedInstances; ///< Set of instances that are anticipated but have not yet started, ordered by the anticipated start time.
};

} // namespace BPMNOS

#endif // BPMNOS_Engine_H
