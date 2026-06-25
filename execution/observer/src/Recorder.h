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
  struct Config {
    std::optional< std::reference_wrapper<std::ostream> > stream;
    bool colored = false; ///< If true, wrap output in ANSI color codes; set false for valid JSON output.
    bool token = true; ///< If true, record token observables.
    bool event = true; ///< If true, record event observables.
    bool message = true; ///< If true, record message observables.
    bool tagged = false; ///< If true, wrap each entry as {"<type>": payload} in both the stream and the in-memory log.
    size_t maxSize = std::numeric_limits<size_t>::max();
  };
  static Config default_config() { return {}; }  // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  Recorder(Config config = default_config());
  ~Recorder();

  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;

  /// Appends an externally supplied json entry to the log (and the attached stream, comma-separated),
  /// independent of the engine observables recorded through notice(). In tagged mode the entry is
  /// wrapped as {tag: json}; otherwise it is stored as given. The caller owns the entry's shape.
  void inject(const std::string& tag, const nlohmann::ordered_json& json);

  nlohmann::ordered_json log; ///< A json object of the entire log.
  /**
   * @brief Returns a json array containing all log entries matching the include object and not matching the exclude object.
   */
  nlohmann::ordered_json find(nlohmann::json include, nlohmann::json exclude = nlohmann::json()) const;
private:
  /// Writes text to the stream, wrapping it in the given color when config.colored is set.
  void emit(const std::string& text, Color::Code color);
  /// Records a json entry to the log and, if a stream is attached, writes it.
  void record(const nlohmann::ordered_json& json, const std::string& type, Color::Code color);
  bool isFirst;
  Config config;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Recorder_H

