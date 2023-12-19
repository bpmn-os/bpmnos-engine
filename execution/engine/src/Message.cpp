#include "Message.h"
#include "Token.h"
#include "model/parser/src/extensionElements/Message.h"

using namespace BPMNOS::Execution;

Message::Message(Token* token)
  : origin(token->node)
{
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::Message>();

  header = messageDefinition->getSenderHeader(token->status);

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

void Message::update(Token* token) const {
  auto& targetContentDefinition = token->node->extensionElements->as<BPMNOS::Model::Message>()->contentMap;
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

