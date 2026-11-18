#ifndef BPMNOS_Execution_Deferrals_H
#define BPMNOS_Execution_Deferrals_H

#include <bpmn++.h>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/DecisionRequest.h"
#include "model/bpmnos/src/extensionElements/Deferral.h"

namespace BPMNOS::Execution {

class SystemState;
struct DataUpdate;
class Message;

/**
 * @brief The deferred decision points of a decision maker, each held back until a reconsideration.
 *
 * A decision point is the performer of sequential activities for their entries, a catch event for the delivery of a
 * message, or a decision task for its choice, and is represented by its token: the performer token, the token at the
 * catch event or the token at the decision task. A token at a sequential activity is resolved to its performer token,
 * so that deferring it and asking whether it is deferred concern its performer. Deferring a decision point defers each
 * of its pending requests, for a performer the entry requests of every token awaiting entry at it, together with the
 * deferral of the model stated for the decision point.
 *
 * A deferral ends at its first reconsideration, which reaches the deferrals as a notification: a new request at the
 * decision point, a new message which may be delivered to a deferred catch event, a data update naming an observed
 * attribute of the deferral, a data update touching an attribute the condition reads after which the condition no
 * longer holds, and a clock tick at which the deferral is no longer admissible. A deferral whose requests have all
 * expired withholds nothing and is removed when next met. An installed system state ends every deferral, since none of
 * the deferred tokens exists in it.
 *
 * Whether a decision point is deferred is a choice of a decision maker, which holds the deferrals and subscribes them to
 * the notifications of the engine, and offers a deferral only where its condition holds and a deferral is admissible.
 */
class Deferrals : public Observer {
public:
  /// The deferred requests of a decision point and the deferral of the model holding them back.
  using Entry = std::tuple< std::vector< std::weak_ptr<const DecisionRequest> >, const BPMNOS::Model::Deferral* >;

  void connect(Notifier* notifier);
  void notice(const Observable* observable) override;

  /**
   * @brief Defers the decision point of the token, a token at a sequential activity being resolved to its performer
   * token.
   *
   * @throws std::runtime_error if the decision point has no pending request or the model states no deferral for it.
   */
  void defer(const Token* token);

  /// @brief Returns true if the decision point of the token is deferred, a token at a sequential activity being
  /// resolved to its performer token.
  bool isDeferred(const Token* token) const;

  /**
   * @brief Returns true if a reconsideration of the deferral can still come: if the deferral depends on time, if the
   * system state awaits an event, or if the data provider of the scenario expects data.
   */
  bool isAdmissible(const BPMNOS::Model::Deferral* deferral) const;

  /// @brief Returns the deferred decision points by their tokens.
  const std::unordered_map< const Token*, Entry >& getDeferrals() const;

private:
  std::unordered_map< const Token*, Entry > deferrals;
  const SystemState* systemState = nullptr;

  void dataUpdate(const DataUpdate* update);
  void message(const Message* message);
  /// Returns true if a request of the entry has not expired, so that its token exists.
  static bool isAlive(const Entry& entry);
  /// Returns the token representing the decision point of the token.
  static const Token* decisionPoint(const Token* token);
  /// Returns the deferral the model states for the decision point represented by the token, nullptr if none.
  static const BPMNOS::Model::Deferral* deferralOf(const Token* token);
  /// Returns the instance of the token representing a decision point.
  static BPMNOS::number instanceOf(const Token* token);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Deferrals_H
