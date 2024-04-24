#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token, Evaluator* evaluator)
  : Event(token)
  , ExitEvent(token)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> ExitDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

nlohmann::ordered_json ExitDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "exit";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  if ( evaluation.has_value() ) {
    jsonObject["evaluation"] = (double)evaluation.value();
  }

  return jsonObject;
}
