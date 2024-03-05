#include "MessageDefinition.h"
#include "Content.h"
#include "ExtensionElements.h"
#include "model/parser/src/xml/bpmnos/tContent.h"

using namespace BPMNOS::Model;

MessageDefinition::MessageDefinition(XML::bpmnos::tMessage* message, AttributeMap& attributeMap)
  : element(message)
  , name( BPMNOS::to_number(message->name.value.value,STRING) )
{
    header.resize(3);
    header[ Index::Name ] = "name";
    header[ Index::Sender ] = "sender";
    header[ Index::Recipient ] = "recipient";

    std::set< std::string > additionalHeader;
    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      auto& key = parameter.name.value.value;
      parameterMap.emplace(key,std::make_unique<Parameter>(&parameter,attributeMap));
      if ( key != "name" && key != "sender" && key != "recipient" ) {
        additionalHeader.insert(key);
      }
    }
    for ( auto& key : additionalHeader ) {
      header.push_back(key);
    }

    for ( XML::bpmnos::tContent& content : element->getChildren<XML::bpmnos::tContent>() ) {
      contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeMap));
    }
}

BPMNOS::Values MessageDefinition::getSenderHeader(const BPMNOS::Values& status) const {
  BPMNOS::Values headerValues;

  for ( auto& key : header ) {
    if ( key == "name" ) {
      headerValues.push_back( name );
    }
    else if ( key == "sender" ) {
      headerValues.push_back( status[BPMNOS::Model::ExtensionElements::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(status, key) );
    }
  }

  return headerValues;
}

BPMNOS::Values MessageDefinition::getRecipientHeader(const BPMNOS::Values& status) const {
  BPMNOS::Values headerValues;

  for ( auto& key : header ) {
    if ( key == "name" ) {
      headerValues.push_back( name );
    }
    else if ( key == "recipient" ) {
      headerValues.push_back( status[BPMNOS::Model::ExtensionElements::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(status, key) );
    }
  }

  return headerValues;
}

std::optional<BPMNOS::number> MessageDefinition::getHeaderValue(const BPMNOS::Values& status, const std::string& key) const {
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
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::COLLECTION ) {
        // use string representation of collection
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
