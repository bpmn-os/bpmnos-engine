#ifndef BPMNOS_Execution_Recorder_H
#define BPMNOS_Execution_Recorder_H

#include <ostream>
#include <bpmn++.h>
#include "Listener.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

/**
 * @brief Class recording all token changes.
 */
class Recorder : public Listener {
public:
  Recorder(size_t maxSize = std::numeric_limits<size_t>::max());
  Recorder(std::ostream &os, size_t maxSize = std::numeric_limits<size_t>::max());
  ~Recorder();

  void update( const Token* token ) override;
  nlohmann::ordered_json log; ///< A json object of the entire log.
  /**
   * @brief Returns a json array containing all log entries matching the include object and not matching the exclude object.
   */
  nlohmann::ordered_json find(nlohmann::json include, nlohmann::json exclude = nlohmann::json()) const;
private:
  std::optional< std::reference_wrapper<std::ostream> > os;
  bool isFirst; 
  size_t maxSize;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Recorder_H

