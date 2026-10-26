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
  , content(other->content)
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

  // the name of the message and the content object the operators of the sender wrote
  assert( token->content && token->content->object );
  content = token->content.value();
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
  jsonObject["name"] = BPMNOS::to_string(content.name,STRING);
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

  // the content object is rendered as JSON, its fields being named as the type of the message names them
  jsonObject["content"] = content.object->jsonify();

  return jsonObject;
}
