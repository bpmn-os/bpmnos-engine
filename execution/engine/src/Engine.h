#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include <map>
#include <vector>
#include "model/data/src/DataProvider.h"
#include "StateMachine.h"

namespace BPMNOS::Execution {

class Engine {
public:
  Engine(BPMNOS::Model::DataProvider* dataProvider);
  BPMNOS::Model::DataProvider* dataProvider;
  void start();
  BPMNOS::number getTimestamp();
protected:
  BPMNOS::number timestamp;
  std::vector< std::unique_ptr<StateMachine> > completedInstances; ///< Vector containing the final states of completed instances (even if failed).
  std::vector< std::unique_ptr<StateMachine> > runningInstances; ///< Vector containing the current state of running instances.
  std::map<BPMNOS::number,BPMNOS::Model::InstanceData*, std::less<number> > anticipatedInstances; ///< Map of instances that are anticipated but have not yet started, ordered by the anticipated start time.
};

} // namespace BPMNOS

#endif // BPMNOS_Engine_H
