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
  for ( XML::bpmnos::tParameter& parameter : element->parameter ) {
    auto& key = parameter.name.value.value;
    parameterMap.emplace(key,std::make_unique<Parameter>(&parameter,attributeRegistry));
    if ( key != "name" && key != "sender" && key != "recipient" ) {
      additionalHeader.insert(key);
    }
  }
  for ( auto& key : additionalHeader ) {
    header.push_back(key);
  }

  for ( XML::bpmnos::tContent& content : element->content ) {
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
std::optional<BPMNOS::number> MessageDefinition::getHeaderValue(const std::string& key, [[maybe_unused]] const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  std::optional<BPMNOS::number> value;
  auto it = parameterMap.find(key);
  if ( it != parameterMap.end() && it->second->expression ) {
    value =  it->second->expression->execute(status,data,globals);
  }
  return value;
}

template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::Values>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const; 
template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::SharedValues>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const; 

