#include "Message.h"
#include "Content.h"
#include "Status.h"
#include "model/parser/src/xml/bpmnos/tMessage.h"
#include "model/parser/src/xml/bpmnos/tContent.h"

using namespace BPMNOS::Model;

Message::Message(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  if ( baseElement->extensionElements.has_value() ) {
    if ( auto message = baseElement->is<XML::bpmn::tMessage>(); message ) {
      name = message->name->get().value.value;
    }

    AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;
    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value.value == "request" ) {
        request = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }

    for ( XML::bpmnos::tContent& content : get<XML::bpmnos::tMessage,XML::bpmnos::tContent>() ) {
      contents.push_back(std::make_unique<Content>(&content,attributeMap));
      contentMap[content.key.value] = contents.rbegin()->get();
    }
  }

}

