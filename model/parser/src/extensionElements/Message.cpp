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

    std::set< std::string > additionalHeader;
    for ( XML::bpmnos::tParameter& parameter : get<XML::bpmnos::tMessage,XML::bpmnos::tParameter>() ) {
      auto& name = parameter.name.value.value;
      parameterMap.emplace(name,std::make_unique<Parameter>(&parameter,attributeMap));
      if ( name != "sender" && name != "recipient" ) {
        additionalHeader.insert(name);
      }
    }
    for ( auto& name : additionalHeader ) {
      header.push_back(name);
    }

    for ( XML::bpmnos::tContent& content : get<XML::bpmnos::tMessage,XML::bpmnos::tContent>() ) {
      contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeMap));
    }
  }

}

BPMNOS::Values Message::getSenderHeader(const BPMNOS::Values& status) const {
  BPMNOS::Values headerValues;
  for ( auto& key : header ) {
    if ( key == "sender" ) {
      headerValues.push_back( status[BPMNOS::Model::Status::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(status, key) );
    }
  }
  return headerValues;
}

BPMNOS::Values Message::getRecipientHeader(const BPMNOS::Values& status) const {
  BPMNOS::Values headerValues;
  for ( auto& key : header ) {
    if ( key == "recipient" ) {
      headerValues.push_back( status[BPMNOS::Model::Status::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(status, key) );
    }
  }
  return headerValues;
}

std::optional<BPMNOS::number> Message::getHeaderValue(const BPMNOS::Values& status, const std::string& key) const {
  auto it = parameterMap.find(key);
  if ( it != parameterMap.end() ) {
    auto& parameter = it->second;
    if ( parameter->attribute.has_value() && status[parameter->attribute->get().index].has_value() ) {
      if ( parameter->attribute->get().type == BPMNOS::ValueType::BOOLEAN ) {
        // use string representation of boolean values
        bool value = (bool)status[parameter->attribute->get().index].value();
        return to_number( value, BPMNOS::ValueType::STRING );
      }
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::INTEGER ) {
        // use string representation of  integer values
        int value = (int)status[parameter->attribute->get().index].value();
        return to_number( std::to_string(value), BPMNOS::ValueType::STRING );
      }
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::DECIMAL ) {
        // use string representation of  decimal values
        double value = (double)status[parameter->attribute->get().index].value();
        return to_number( std::to_string(value), BPMNOS::ValueType::STRING );
      }
      return status[parameter->attribute->get().index].value();
    }
    else if ( parameter->value.has_value() ) {
      return to_number( parameter->value->get().value, BPMNOS::ValueType::STRING );
    }
  }
  return std::nullopt;
}
