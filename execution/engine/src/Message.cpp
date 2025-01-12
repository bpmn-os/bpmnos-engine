#include "Message.h"
#include "Token.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"

using namespace BPMNOS::Execution;

Message::Message(Token* token, size_t index)
  : state(State::CREATED)
  , origin(token->node)
  , waitingToken(nullptr)
{
  if ( token->node->represents<BPMN::SendTask>() ) {
    waitingToken = token;
  }
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(index);

  auto& attributeRegistry = token->getAttributeRegistry();

  header = messageDefinition->getSenderHeader(attributeRegistry,token->status,*token->data,token->globals);
  if ( header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].has_value() ) {
    recipient = header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].value();
  }

  for (auto& [key,contentDefinition] : messageDefinition->contentMap) {
    if ( token->status[contentDefinition->attribute->index].has_value() ) {
      contentValueMap.emplace( key, attributeRegistry.getValue(contentDefinition->attribute,token->status,*token->data,token->globals) );
    }
    else {
      contentValueMap.emplace( key, std::nullopt );
    }
  }
}

bool Message::matches(const BPMNOS::Values& otherHeader) const {
  if ( header.size() != otherHeader.size() ) {
    return false;
  }

  for ( size_t i = 0; i < header.size(); i++ ) {
    if ( header[i].has_value() && otherHeader[i].has_value() && header[i].value() != otherHeader[i].value() ) {
      return false;
    }
  }

  return true;
}

nlohmann::ordered_json Message::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["origin"] = origin->id;
  jsonObject["state"] = stateName[(int)state];

  auto& messageDefinition = origin->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front();
  size_t i = 0;
  for ( auto headerName : messageDefinition->header ) {
    if ( !header[i].has_value() ) {
      jsonObject["header"][headerName] = nullptr ;
    }
    else {
      jsonObject["header"][headerName] = BPMNOS::to_string(header[i].value(),STRING);
    }
    ++i;
  }

  for ( auto& [key,contentValue] : contentValueMap ) {
//std::cerr << "Key: " << key << std::endl;  
    if ( std::holds_alternative< std::optional<number> >(contentValue) && std::get< std::optional<number> >(contentValue).has_value() ) {
//std::cerr << "has value" << std::endl;  
      auto type = BPMNOS::ValueType::STRING;
      if ( auto it = messageDefinition->contentMap.find(key); it != messageDefinition->contentMap.end() ) {
        type = it->second->attribute->type;
      }
      number value = std::get< std::optional<number> >(contentValue).value();
      jsonObject["content"][key] = BPMNOS::to_string(value,type);
    }
    else if (std::holds_alternative<std::string>(contentValue)) {
//std::cerr << "has string" << std::endl;  
      jsonObject["content"][key] = std::get< std::string >(contentValue);
    }
    else {
//std::cerr << "else" << std::endl;  
      jsonObject["content"][key] = nullptr;
    }
  }

  return jsonObject;
}


template <typename DataType>
void Message::apply(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  auto& targetContentDefinition = node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(status)->contentMap;

  size_t counter = 0;
  for (auto& [key,contentValue] : contentValueMap) {
    if ( auto it = targetContentDefinition.find(key); it != targetContentDefinition.end() ) {
      auto& [_,definition] = *it;
      auto attribute = definition->attribute;
//std::cerr << "Attribute: " << attribute.name << "/" << attribute.index << std::endl;
      if ( std::holds_alternative< std::optional<number> >(contentValue) && std::get< std::optional<number> >(contentValue).has_value() ) {
        // use attribute value sent in message
        attributeRegistry.setValue(attribute, status, data, globals, std::get< std::optional<number> >(contentValue).value() );
      }
      else if (std::holds_alternative<std::string>(contentValue)) {
        // use default value of sender
        Value value = std::get< std::string >(contentValue);
        attributeRegistry.setValue(attribute, status, data, globals, BPMNOS::to_number(value,attribute->type) );
      }
      else {
        attributeRegistry.setValue(attribute, status, data, globals, std::nullopt );
      }
    }
    else {
      // key in message content, but not in recipient content
      counter++;
    }
  }

  if ( targetContentDefinition.size() > contentValueMap.size() - counter ) {
    // recipient has keys in content that are not in message content
    for (auto& [key,definition] : targetContentDefinition) {
      if ( !contentValueMap.contains(key) ) {
        // key in recipient content, but not in message content
        attributeRegistry.setValue(definition->attribute, status, data, globals, std::nullopt );
      }
    }
  }
}

template void Message::apply<BPMNOS::Values>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, Values& data, BPMNOS::Values& globals) const;
template void Message::apply<BPMNOS::SharedValues>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, SharedValues& data, BPMNOS::Values& globals) const;

