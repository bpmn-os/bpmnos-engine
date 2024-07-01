#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(const Token* token, Values choices)
  : Event(token)
  , choices(choices)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json ChoiceEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "choice";
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

  return jsonObject;
}
