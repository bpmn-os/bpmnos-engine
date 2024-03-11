#ifndef BPMNOS_Execution_Recorder_H
#define BPMNOS_Execution_Recorder_H

#include <ostream>
#include <bpmn++.h>
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Observer.h"
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

namespace Color {
  enum Code {
    FG_DEFAULT = 39,
    FG_BLACK = 30,
    FG_RED = 31,
    FG_GREEN = 32,
    FG_YELLOW = 33,
    FG_BLUE = 34,
    FG_MAGENTA = 35,
    FG_CYAN = 36,
    FG_LIGHT_GRAY = 37,
    FG_DARK_GRAY = 90,
    FG_LIGHT_RED = 91,
    FG_LIGHT_GREEN = 92,
    FG_LIGHT_YELLOW = 93,
    FG_LIGHT_BLUE = 94,
    FG_LIGHT_MAGENTA = 95,
    FG_LIGHT_CYAN = 96,
    FG_WHITE = 97,
    BG_RED = 41,
    BG_GREEN = 42,
    BG_BLUE = 44,
    BG_DEFAULT = 49
  };
  class Modifier {
    Code code;
  public:
    Modifier(Code pCode) : code(pCode) {}
    friend std::ostream& operator<<(std::ostream& os, const Modifier& mod) {
      return os << "\033[" << mod.code << "m";
    }
  };
}

/**
 * @brief Class recording all token changes.
 */
class Recorder : public Observer {
public:
  Recorder(size_t maxSize = std::numeric_limits<size_t>::max());
  Recorder(std::ostream &os, size_t maxSize = std::numeric_limits<size_t>::max());
  ~Recorder();

  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;

  BPMNOS::number objective; ///< The global objective.
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

