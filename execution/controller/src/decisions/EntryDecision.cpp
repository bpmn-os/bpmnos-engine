#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, Evaluator* evaluator)
  : Event(token)
  , EntryEvent(token)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> EntryDecision::evaluate() {
  reward = evaluator->evaluate(this);
  return reward;
}

nlohmann::ordered_json EntryDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "entry";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  if ( reward.has_value() ) {
    jsonObject["reward"] = (double)reward.value();
  }

  return jsonObject;
}
