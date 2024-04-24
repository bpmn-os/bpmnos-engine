#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, const Message* message, Evaluator* evaluator)
  : Event(token)
  , MessageDeliveryEvent(token,message)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> MessageDeliveryDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

nlohmann::ordered_json MessageDeliveryDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "messagedelivery";
  jsonObject["nodeId"] = token->node->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["state"] = Token::stateName[(int)token->state];

  if ( auto object = message.lock() ) {
    jsonObject["message"] = object->jsonify();
  }
  
  if ( evaluation.has_value() ) {
    jsonObject["evaluation"] = (double)evaluation.value();
  }

  return jsonObject;
}
