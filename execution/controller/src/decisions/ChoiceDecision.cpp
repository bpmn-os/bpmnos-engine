#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values updatedStatus, Evaluator* evaluator)
  : Event(token)
  , ChoiceEvent(token, updatedStatus)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> ChoiceDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

nlohmann::ordered_json ChoiceDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "choice";
  jsonObject["nodeId"] = token->node->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["state"] = Token::stateName[(int)token->state];

  if ( evaluation.has_value() ) {
    jsonObject["evaluation"] = (double)evaluation.value();
  }

  return jsonObject;
}
