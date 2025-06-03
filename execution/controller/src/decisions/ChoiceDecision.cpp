#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values choices, Evaluator* evaluator)
  : Event(token)
  , ChoiceEvent(token, choices)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> ChoiceDecision::evaluate() {
  reward = evaluator->evaluate(this);
  return reward;
}

nlohmann::ordered_json ChoiceDecision::jsonify() const {
  nlohmann::ordered_json jsonObject;


  jsonObject["decision"] = "choice";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  jsonObject["choices"] = nlohmann::ordered_json();
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    auto attribute = extensionElements->choices[i]->attribute;
    if ( !choices[i].has_value() ) {
      jsonObject["choices"][attribute->name] = nullptr ;
    }
    else if ( attribute->type == BOOLEAN) {
      bool value = (bool)choices[i].value();
      jsonObject["choices"][attribute->name] = value ;
    }
    else if ( attribute->type == INTEGER) {
      int value = (int)choices[i].value();
      jsonObject["choices"][attribute->name] = value ;
    }
    else if ( attribute->type == DECIMAL) {
      double value = (double)choices[i].value();
      jsonObject["choices"][attribute->name] = value ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(choices[i].value(),attribute->type);
      jsonObject["choices"][attribute->name] = value ;
    }
    else if ( attribute->type == COLLECTION) {
      std::string value = BPMNOS::to_string(choices[i].value(),attribute->type);
      jsonObject["choices"][attribute->name] = value ;
    }
  }

  if ( reward.has_value() ) {
    jsonObject["reward"] = (double)reward.value();
  }

  return jsonObject;
}
