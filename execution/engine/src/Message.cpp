#include "Message.h"
#include "Token.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

Message::Message(const Message* other, Token* waitingToken)
  : state(other->state)
  , origin(other->origin)
  , waitingToken(waitingToken)
  , recipient(other->recipient)
  , header(other->header)
  , contentValueMap(other->contentValueMap)
{
}

Message::Message(Token* token)
  : state(State::CREATED)
  , origin(token->node->as<BPMN::FlowNode>())
  , waitingToken(nullptr)
{
  if ( token->node->represents<BPMN::SendTask>() ) {
    waitingToken = token;
  }
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition();

  auto& attributeRegistry = token->getAttributeRegistry();

  header = messageDefinition->getSenderHeader(attributeRegistry,token->status,*token->data,token->getInstanceId());
  if ( header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].has_value() ) {
    recipient = header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].value();
  }

  for (auto& [key,contentDefinition] : messageDefinition->contentMap) {
    auto attribute = contentDefinition->attribute;
    if ( attribute->isObject() ) {
      // the object is shared, a later write by the sender copying it
      contentValueMap.emplace( key, attributeRegistry.getObject(attribute,token->status,*token->data) );
    }
    else {
      contentValueMap.emplace( key, attributeRegistry.getValue(attribute,token->status,*token->data) );
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

  auto messageDefinition = origin->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition();
  size_t i = 0;
  for ( auto& [key,type] : messageDefinition->header ) {
    if ( !header[i].has_value() ) {
      jsonObject["header"][key] = nullptr ;
    }
    else {
      // a value is held only where the parameter states one, and a parameter stating a value states a type
      assert( type.has_value() );
      jsonObject["header"][key] = BPMNOS::to_string(header[i].value(),type.value());
    }
    ++i;
  }

  for ( auto& [key,contentValue] : contentValueMap ) {
//std::cerr << "Key: " << key << std::endl;  
    if ( std::holds_alternative< std::optional<number> >(contentValue) && std::get< std::optional<number> >(contentValue).has_value() ) {
//std::cerr << "has value" << std::endl;  
      auto it = messageDefinition->contentMap.find(key);
      if ( it == messageDefinition->contentMap.end() ) {
        // the content was filled from this very definition, so a key it does not know is no value to render
        throw std::runtime_error("Message: unknown content key '" + key + "'");
      }
      number value = std::get< std::optional<number> >(contentValue).value();
      jsonObject["content"][key] = BPMNOS::to_string(value,it->second->attribute->type);
    }
    else if (std::holds_alternative<std::string>(contentValue)) {
//std::cerr << "has string" << std::endl;  
      jsonObject["content"][key] = std::get< std::string >(contentValue);
    }
    else if ( std::holds_alternative< std::shared_ptr<const BPMNOS::Object> >(contentValue) ) {
      // an object is rendered as JSON
      jsonObject["content"][key] = BPMNOS::to_json( *std::get< std::shared_ptr<const BPMNOS::Object> >(contentValue) );
    }
    else {
//std::cerr << "else" << std::endl;  
      jsonObject["content"][key] = nullptr;
    }
  }

  return jsonObject;
}


template <typename DataType>
BPMNOS::number Message::apply(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Status& status, DataType& data) const {
  auto& targetContentDefinition = node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition()->contentMap;

  BPMNOS::number objectiveChange = 0;
  size_t counter = 0;
  for (auto& [key,contentValue] : contentValueMap) {
    if ( auto it = targetContentDefinition.find(key); it != targetContentDefinition.end() ) {
      objectiveChange += applyContent(attributeRegistry, key, it->second->attribute, &contentValue, status, data);
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
        objectiveChange += applyContent<DataType>(attributeRegistry, key, definition->attribute, nullptr, status, data);
      }
    }
  }
  return objectiveChange;
}

template BPMNOS::number Message::apply<BPMNOS::Data>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Status& status, Data& data) const;
template BPMNOS::number Message::apply<BPMNOS::SharedData>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Status& status, SharedData& data) const;

