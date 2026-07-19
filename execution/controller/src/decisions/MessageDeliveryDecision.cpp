#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const DecisionRequest* request, const Message* message, Evaluator* evaluator)
  : Event(request->token)
  , MessageDeliveryEvent(request->token,message)
  , Decision(request, evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

bool MessageDeliveryDecision::expired() const {
  // Diamond: MessageDeliveryEvent and Decision both override expired(); merge request (covers token) and
  // the independently-withdrawable message.
  return request.expired() || message.expired();
}

std::shared_ptr<Evaluation> MessageDeliveryDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

nlohmann::ordered_json MessageDeliveryDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "messagedelivery";
  auto token = this->token.lock();
  auto message = this->message.lock();
  if ( !token || !message || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;
  jsonObject["message"] = message->jsonify();
  
  if ( reward().has_value() ) {
    jsonObject["reward"] = (double)reward().value();
  }

  return jsonObject;
}
