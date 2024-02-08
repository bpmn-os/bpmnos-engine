#include "Message.h"
#include "Token.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"

using namespace BPMNOS::Execution;

Message::Message(Token* token, size_t index)
  : origin(token->node)
  , waitingToken(nullptr)
{
  if ( token->node->represents<BPMN::SendTask>() ) {
    waitingToken = token;
  }

  if ( index >= token->node->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions.size() ) {
    throw std::runtime_error("Message: no message with index " + std::to_string(index) + " provided for '" +  token->node->id + "'" );
  }
  auto& messageDefinition = token->node->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions[index];

  header = messageDefinition->getSenderHeader(token->status);
  if ( header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].has_value() ) {
    recipient = BPMNOS::to_string(header[ BPMNOS::Model::MessageDefinition::Index::Recipient ].value(),STRING);
  }

  for (auto& [key,contentDefinition] : messageDefinition->contentMap) {
    if ( contentDefinition->attribute.has_value() && token->status[contentDefinition->attribute->get().index].has_value() ) {
      contentValueMap.emplace( key, token->status[contentDefinition->attribute->get().index] );
    }
    else if ( contentDefinition->value.has_value() ) {
      contentValueMap.emplace( key, contentDefinition->value.value() );
    }
    else {
      contentValueMap.emplace( key, std::nullopt );
    }
  }
}

bool Message::matches(const BPMNOS::Values& otherHeader) {
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
// TODO!
  auto& messageDefinition = origin->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions.front();
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
    if ( std::get< std::optional<number> >(contentValue).has_value() ) {
      auto type = BPMNOS::ValueType::STRING;
      if ( auto it = messageDefinition->contentMap.find(key); it != messageDefinition->contentMap.end() ) {
        type = it->second->attribute->get().type;
      }
      number value = std::get< std::optional<number> >(contentValue).value();
      jsonObject["content"][key] = BPMNOS::to_string(value,type);
    }
    else if (std::holds_alternative<std::string>(contentValue)) {
      jsonObject["content"][key] = std::get< std::string >(contentValue);
    }
    else {
      jsonObject["content"][key] = nullptr;
    }
  }

  return jsonObject;
}


void Message::update(Token* token) const {
  size_t index = 0;

  if ( auto receiveTask = token->node->represents<BPMN::ReceiveTask>();
    receiveTask &&
    receiveTask->loopCharacteristics.has_value()
  ) {
    auto statusExtension = receiveTask->extensionElements->represents<BPMNOS::Model::Status>();
    assert(statusExtension);

    // multi-instance receive task
    if ( !statusExtension->loopIndex.has_value() || !statusExtension->loopIndex->get()->attribute.has_value() ) {
      throw std::runtime_error("Message: receive tasks with loop characteristics requires attribute holding loop index");
    }
    size_t attributeIndex = statusExtension->loopIndex->get()->attribute.value().get().index;
    if ( !token->status[attributeIndex].has_value() ) { 
      throw std::runtime_error("Message: cannot find loop index for receive tasks with loop characteristics");
    }
    index = (size_t)(int)token->status[index].value();
  }
  
  if ( index >= token->node->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions.size() ) {
    throw std::runtime_error("Message: no message with index " + std::to_string(index) + " provided for '" +  token->node->id + "'" );
  }

  auto& targetContentDefinition = token->node->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions[index]->contentMap;

  size_t counter = 0;
  for (auto& [key,contentValue] : contentValueMap) {
    if ( auto it = targetContentDefinition.find(key); it != targetContentDefinition.end() ) {
      auto& [key,definition] = *it;
      if ( !definition->attribute.has_value() ) {
        throw std::runtime_error("Message: cannot receive content without attribute");
      }
      auto& attribute = definition->attribute->get();

      if ( std::get< std::optional<number> >(contentValue).has_value() ) {
        // use attribute value sent in message
        token->status[attribute.index] = std::get< std::optional<number> >(contentValue).value();
      }
      else if (std::holds_alternative<std::string>(contentValue)) {
        // use default value of sender
        Value value = std::get< std::string >(contentValue);
        token->status[attribute.index] = BPMNOS::to_number(value,attribute.type);
      }
      else if ( definition->value.has_value() ) {
        // use default value or recipient
        token->status[attribute.index] = BPMNOS::to_number(definition->value.value(),attribute.type);
      }
      else {
        token->status[attribute.index] = std::nullopt;
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
        auto& attribute = definition->attribute->get();

        if ( definition->value.has_value() ) {
          // use default value or recipient
          token->status[attribute.index] = BPMNOS::to_number(definition->value.value(),attribute.type);
        }
        else {
          token->status[attribute.index] = std::nullopt;
        }
      }
    }
  }
}

