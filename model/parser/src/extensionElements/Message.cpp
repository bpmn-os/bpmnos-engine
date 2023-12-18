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

    header = {"sender","recipient"};

    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      parameterMap.emplace(parameter.name.value,std::make_unique<Parameter>(&parameter,attributeMap));
      header.insert(parameter.name.value);
    }

    for ( XML::bpmnos::tContent& content : get<XML::bpmnos::tMessage,XML::bpmnos::tContent>() ) {
      contentMap.emplace(content.key.value,std::make_unique<Content>(&content,attributeMap));
    }
  }

}

BPMNOS::Values Message::getHeaderValues(BPMNOS::Values& status) const {
  BPMNOS::Values headerValues;
  for ( auto& key : header ) {
    auto it = parameterMap.find(key);
    if ( it != parameterMap.end() ) {
      auto& parameter = it->second;
      if ( parameter->attribute.has_value() && status[parameter->attribute->get().index].has_value() ) {
        headerValues.push_back( status[parameter->attribute->get().index].value() );
      }
      else if ( parameter->value.has_value() ) {
        headerValues.push_back( to_number( parameter->value->get().value, BPMNOS::ValueType::STRING ) );
      }
      else {
        headerValues.push_back( std::nullopt );
      }
    }
    else {
      // key is either 'sender' or 'recipient' and must refer to the respective instance identifier
      headerValues.push_back( status[BPMNOS::Model::Status::Index::Instance] );
    }
  }
  return headerValues;
}

