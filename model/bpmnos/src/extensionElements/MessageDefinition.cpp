#include "MessageDefinition.h"
#include "Content.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tContent.h"

using namespace BPMNOS::Model;

MessageDefinition::MessageDefinition(XML::bpmnos::tMessage* message, const AttributeRegistry& attributeRegistry)
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
    parameterMap.emplace(key,std::make_unique<Parameter>(&parameter,attributeRegistry));
    if ( key != "name" && key != "sender" && key != "recipient" ) {
      additionalHeader.insert(key);
    }
  }
  for ( auto& key : additionalHeader ) {
    header.push_back(key);
  }

  for ( XML::bpmnos::tContent& content : element->getChildren<XML::bpmnos::tContent>() ) {
    contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeRegistry));
  }
}

template <typename DataType>
BPMNOS::Values MessageDefinition::getSenderHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::Values headerValues;

  for ( auto& key : header ) {
    if ( key == "name" ) {
      headerValues.push_back( name );
    }
    else if ( key == "sender" ) {
      headerValues.push_back( data[BPMNOS::Model::ExtensionElements::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(key, attributeRegistry, status, data, globals) );
    }
  }

  return headerValues;
}

template  BPMNOS::Values MessageDefinition::getSenderHeader<BPMNOS::Values>(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template  BPMNOS::Values MessageDefinition::getSenderHeader<BPMNOS::SharedValues>(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

template <typename DataType>
BPMNOS::Values MessageDefinition::getRecipientHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::Values headerValues;

  for ( auto& key : header ) {
    if ( key == "name" ) {
      headerValues.push_back( name );
    }
    else if ( key == "recipient" ) {
      headerValues.push_back( data[BPMNOS::Model::ExtensionElements::Index::Instance] );
    }
    else {
      headerValues.push_back( getHeaderValue(key, attributeRegistry, status, data, globals) );
    }
  }

  return headerValues;
}

template  BPMNOS::Values MessageDefinition::getRecipientHeader<BPMNOS::Values>(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template  BPMNOS::Values MessageDefinition::getRecipientHeader<BPMNOS::SharedValues>(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

template <typename DataType>
std::optional<BPMNOS::number> MessageDefinition::getHeaderValue(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  auto it = parameterMap.find(key);
  if ( it != parameterMap.end() ) {
    auto& parameter = it->second;
    auto value = ( parameter->attribute.has_value() ? attributeRegistry.getValue(&parameter->attribute->get(),status,data,globals) : std::nullopt );
    if ( value.has_value() ) {
      if ( parameter->attribute->get().type == BPMNOS::ValueType::BOOLEAN ) {
        // use string representation of boolean values
        return to_number( (bool)value.value(), BPMNOS::ValueType::STRING );
      }
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::INTEGER ) {
        // use string representation of  integer values
        return to_number( std::to_string((int)value.value()), BPMNOS::ValueType::STRING );
      }
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::DECIMAL ) {
        // use string representation of  decimal values
        return to_number( std::to_string((double)value.value()), BPMNOS::ValueType::STRING );
      }
      else if ( parameter->attribute->get().type == BPMNOS::ValueType::COLLECTION ) {
        // use string representation of collection
        return to_number( std::to_string((double)value.value()), BPMNOS::ValueType::STRING );
      }
      return value.value();
    }
    else if ( parameter->value.has_value() ) {
      return to_number( parameter->value->get().value, BPMNOS::ValueType::STRING );
    }
  }
  return std::nullopt;
}

template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::Values>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const; 
template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::SharedValues>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const; 

