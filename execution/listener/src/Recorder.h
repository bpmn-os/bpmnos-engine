#ifndef BPMNOS_Execution_Recorder_H
#define BPMNOS_Execution_Recorder_H

#include <bpmn++.h>
#include "execution/engine/src/Listener.h"

namespace BPMNOS::Execution {

/**
 * @brief Class recording all token changes.
 */
class Recorder : public Listener {
public:
  Recorder();
  void update( const Token* token ) override;
  nlohmann::json log; ///< A json object of the entire log.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Recorder_H

