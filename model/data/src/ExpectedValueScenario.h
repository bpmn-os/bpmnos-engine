#ifndef BPMNOS_Model_ExpectedValueScenario_H
#define BPMNOS_Model_ExpectedValueScenario_H

#include "StaticScenario.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <map>
#include <memory>

namespace BPMNOS::Model {

class ExpectedValueDataProvider;

/**
 * @brief Structure representing a completion expression.
 */
struct ExpectedCompletionExpression {
  const Attribute* attribute;
  std::unique_ptr<Expression> expression;
};

/**
 * @brief A scenario that extends StaticScenario with completion expression support.
 *
 * Derives from StaticScenario (so CPModel accepts it) but adds support for
 * COMPLETION expressions evaluated using expected values.
 */
class ExpectedValueScenario : public StaticScenario {
  friend class ExpectedValueDataProvider;

public:
  ExpectedValueScenario(
    const Model* model,
    BPMNOS::number earliestInstantiationTime,
    BPMNOS::number latestInstantiationTime,
    const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap
  );

  /**
   * @brief Store the completion status when a task enters BUSY state.
   *
   * Evaluates COMPLETION expressions using expected values and stores the result.
   */
  void setTaskCompletionStatus(
    const BPMNOS::number instanceId,
    const BPMN::Node* task,
    BPMNOS::Values status
  ) const override;

protected:
  void addCompletionExpression(
    const BPMNOS::number instanceId,
    const BPMN::Node* task,
    ExpectedCompletionExpression&& expr
  );

  /// Completion expressions per (instance, node)
  mutable std::map<size_t, std::map<const BPMN::Node*, std::vector<ExpectedCompletionExpression>>> completionExpressions;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpectedValueScenario_H
