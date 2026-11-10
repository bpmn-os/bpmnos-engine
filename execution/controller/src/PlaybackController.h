#ifndef BPMNOS_Execution_PlaybackController_H
#define BPMNOS_Execution_PlaybackController_H

#include <bpmn++.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "Controller.h"
#include "Evaluator.h"
#include "Decision.h"
#include "evaluators/LocalEvaluator.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief A controller playing back the decisions of a log.
 *
 * The log is the JSON output of the engine, an array of records each tagged by its kind, of which the
 * controller reads the clock ticks and the decisions and ignores every other record. A decision is a record
 * tagged `event` whose object states its kind by the key `decision`: the entry of a child of a sequential ad-hoc
 * subprocess, the choices made at a decision task, or the delivery of a message that is not directly addressed.
 * Its reward is not read. A decision belongs to the time of the last clock tick before it, and a decision before
 * the first clock tick to the time the run starts. The engine carries out everything that is not a decision
 * itself, so that a log recorded by the engine and a log stating its decisions alone are played back alike.
 *
 * A decision is due at the first time the run reaches that is not earlier than its own time; a later time is a
 * deviation, which is reported. At every fetch, the controller dispatches the first due decision, in the order of
 * the log, whose request is pending, matched by process, instance and node, and for a message delivery by a
 * message among the candidates of the request agreeing with the origin, the header and the content the log
 * states. An entry waits while an entry listed before it for the same performer, the performer node of the
 * sequential ad-hoc subprocess within the same instance, has not been dispatched, so that the order of the entries
 * of every performer is kept. Every other decision is dispatched as soon as its request is pending. A decision is
 * evaluated by the evaluator given before it is dispatched, which records its reward.
 *
 * The playback stops at the first mismatch, by a termination event: a due decision not dispatched before time
 * advances, a decision found infeasible by the evaluator, a choice lacking the value of a choice attribute, or an
 * entry of an activity whose entry is not decided.
 */
class PlaybackController : public Controller {
public:
  /**
   * @brief A decision read from a log.
   */
  struct LoggedDecision {
    enum class Kind { Entry, Choice, MessageDelivery };
    Kind kind = Kind::Entry;
    BPMNOS::number time; ///< The time the decision belongs to, the lowest representable time before the first clock tick
    std::string processId; ///< The process of the deciding token
    std::string instanceId; ///< The instance of the deciding token
    std::string nodeId; ///< The node at which the deciding token resides
    nlohmann::ordered_json choices; ///< The values chosen, by attribute name, for a choice
    nlohmann::ordered_json message; ///< The message as the log states it, for a message delivery
    std::string origin; ///< The node sending the message, for a message delivery
    std::optional<std::string> sender; ///< The instance sending the message, for a message delivery stating it
  };
  /**
   * @brief A decision dispatched later than its time, the time not being one the run reaches.
   */
  struct Deviation {
    const LoggedDecision* decision;
    BPMNOS::number time; ///< The time at which the decision is dispatched
  };
  /**
   * @brief The decision at which the playback stops.
   */
  struct Mismatch {
    const LoggedDecision* decision;
    BPMNOS::number time; ///< The time at which the mismatch is found
    std::string reason;
  };
  /**
   * @brief Constructs a controller playing back the decisions of a log.
   *
   * @param log The JSON output of the engine, or an array of clock ticks and decisions alone
   * @param evaluator The evaluator evaluating every decision before it is dispatched
   * @throws std::runtime_error if the log is not an array, or a decision is of an unknown kind or lacks a field
   * it requires
   */
  PlaybackController(const nlohmann::ordered_json& log, std::shared_ptr<Evaluator> evaluator = std::make_shared<LocalEvaluator>());
  /**
   * @brief Returns the decisions read, grouped by the time they belong to, in the order of the log.
   *
   * The decisions before the first clock tick are found at the lowest representable time.
   */
  const std::map< BPMNOS::number, std::vector<LoggedDecision> >& getDecisions() const;
  /// @brief Returns the number of decisions dispatched so far.
  size_t getDispatched() const;
  /// @brief Returns the decisions dispatched later than their time.
  const std::vector<Deviation>& getDeviations() const;
  /// @brief Returns the mismatch at which the playback stopped, if any.
  const std::optional<Mismatch>& getMismatch() const;
  /// @brief Returns true if every decision of the log has been dispatched without a mismatch.
  bool isComplete() const;
  /// @brief Returns the result of the playback: the numbers of decisions and of those dispatched, the deviations, and the mismatch.
  nlohmann::ordered_json jsonify() const;
  /// @brief Returns a decision as the log states it.
  static nlohmann::ordered_json jsonify(const LoggedDecision& decision);
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;
protected:
  std::shared_ptr<Evaluator> evaluator;
  std::map< BPMNOS::number, std::vector<LoggedDecision> > decisions;
  std::vector<const LoggedDecision*> sequence; ///< The decisions ordered by their time, and within it by the log
  std::vector<bool> dispatched; ///< Whether the decision at the same position of the sequence has been dispatched
  std::vector< std::optional<BPMNOS::number> > due; ///< The time at which the decision at the same position of the sequence became due
  size_t dispatchedCount = 0;
  std::vector<Deviation> deviations;
  std::optional<Mismatch> mismatch;
  std::unordered_map<std::string, const BPMN::Node*> nodes; ///< The nodes looked up by their identifiers
  const BPMN::Node* findNode(const SystemState* systemState, const std::string& nodeId);
  std::shared_ptr<Event> stop(const LoggedDecision* decision, BPMNOS::number time, std::string reason);
  std::shared_ptr<Decision> createDecision(const SystemState* systemState, const LoggedDecision& decision);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_PlaybackController_H
