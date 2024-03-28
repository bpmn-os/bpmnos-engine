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

  if ( index >= token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.size() ) {
    throw std::runtime_error("Message: no message with index " + std::to_string(index) + " provided for '" +  token->node->id + "'" );
  }
  auto& messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions[index];

  auto& attributeRegistry = token->getAttributeRegistry();
  // TODO: token->owner->status
  header = messageDefinition->getSenderHeader(attributeRegistry,token->status,token->data);
  if ( header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].has_value() ) {
    recipient = BPMNOS::to_string(header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].value(),STRING);
  }

  for (auto& [key,contentDefinition] : messageDefinition->contentMap) {
    if ( contentDefinition->attribute.has_value() && token->status[contentDefinition->attribute->get().index].has_value() ) {
      contentValueMap.emplace( key, attributeRegistry.getValue(&contentDefinition->attribute->get(),token->status,token->data) );
    }
    else if ( contentDefinition->value.has_value() ) {
      contentValueMap.emplace( key, contentDefinition->value.value() );
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
        type = it->second->attribute->get().type;
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
void Message::apply(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, DataType& data) const {
//void Message::apply(const BPMN::FlowNode* node, BPMNOS::Values& status) const {
  size_t index = 0;

  if ( auto receiveTask = node->represents<BPMN::ReceiveTask>();
    receiveTask &&
    receiveTask->loopCharacteristics.has_value()
  ) {
    auto extensionElements = receiveTask->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
    assert(extensionElements);

    // multi-instance receive task
    if ( !extensionElements->loopIndex.has_value() || !extensionElements->loopIndex->get()->attribute.has_value() ) {
      throw std::runtime_error("Message: receive tasks with loop characteristics requires attribute holding loop index");
    }
    
    // TODO:check
    size_t attributeIndex = extensionElements->loopIndex->get()->attribute.value().get().index;
    if ( !status[attributeIndex].has_value() ) { 
      throw std::runtime_error("Message: cannot find loop index for receive tasks with loop characteristics");
    }
    index = (size_t)(int)status[index].value();
  }
  
  if ( index >= node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.size() ) {
    throw std::runtime_error("Message: no message with index " + std::to_string(index) + " provided for '" +  node->id + "'" );
  }

  auto& targetContentDefinition = node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions[index]->contentMap;

  size_t counter = 0;
  for (auto& [key,contentValue] : contentValueMap) {
    if ( auto it = targetContentDefinition.find(key); it != targetContentDefinition.end() ) {
      auto& [key,definition] = *it;
      if ( !definition->attribute.has_value() ) {
        throw std::runtime_error("Message: cannot receive content without attribute");
      }
      auto attribute = &definition->attribute->get();
//std::cerr << "Attribute: " << attribute.name << "/" << attribute.index << std::endl;
      if ( std::holds_alternative< std::optional<number> >(contentValue) && std::get< std::optional<number> >(contentValue).has_value() ) {
        // use attribute value sent in message
        attributeRegistry.setValue(attribute, status, data, std::get< std::optional<number> >(contentValue).value() );
      }
      else if (std::holds_alternative<std::string>(contentValue)) {
        // use default value of sender
        Value value = std::get< std::string >(contentValue);
        attributeRegistry.setValue(attribute, status, data, BPMNOS::to_number(value,attribute->type) );
//        status[attribute.index] = BPMNOS::to_number(value,attribute.type);
      }
      else if ( definition->value.has_value() ) {
        // use default value or recipient
        attributeRegistry.setValue(attribute, status, data, BPMNOS::to_number(definition->value.value(),attribute->type) );
//        status[attribute.index] = BPMNOS::to_number(definition->value.value(),attribute.type);
      }
      else {
        attributeRegistry.setValue(attribute, status, data, std::nullopt );
//        status[attribute.index] = std::nullopt;
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
        if ( !definition->attribute.has_value() ) {
          throw std::runtime_error("Message: cannot receive content without attribute");
        }
        auto attribute = &definition->attribute->get();

        if ( definition->value.has_value() ) {
          // use default value or recipient
          attributeRegistry.setValue(attribute, status, data, BPMNOS::to_number(definition->value.value(),attribute->type) );
//          status[attribute.index] = BPMNOS::to_number(definition->value.value(),attribute.type);
        }
        else {
          attributeRegistry.setValue(attribute, status, data, std::nullopt );
//          status[attribute.index] = std::nullopt;
        }
      }
    }
  }
}

template void Message::apply<BPMNOS::Values>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, Values& data) const;
template void Message::apply<BPMNOS::Globals>(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, Globals& data) const;

