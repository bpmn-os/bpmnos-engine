#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/DecisionRequest.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const DecisionRequest* request, Evaluator* evaluator)
  : Event(request->token)
  , EntryEvent(request->token)
  , Decision(request, evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::shared_ptr<Evaluation> EntryDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

nlohmann::ordered_json EntryDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["decision"] = "entry";
  auto token = this->token.lock();
  if ( !token || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  if ( reward().has_value() ) {
    jsonObject["reward"] = (double)reward().value();
  }

  return jsonObject;
}
