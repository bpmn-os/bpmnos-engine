#include "MessageDefinition.h"
#include "Content.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tContent.h"
#include "model/utility/src/string_utility.h"
#include <map>

using namespace BPMNOS::Model;

namespace {

/**
 * @brief Returns the type of the values held under a header key, and no type where the parameter states
 * no value, the entry then holding none.
 *
 * A header parameter states the name of a declared attribute or a quoted string and nothing else, so
 * that the type is known when the model is read. Both are one node of the compiled expression, a
 * variable in the first case and a literal in the second, and the text tells a quoted string from a
 * number, the scan having turned the one into the other. Every other shape is refused.
 */
std::optional<BPMNOS::ValueType> getHeaderType(const XML::bpmnos::tParameter& element, const Parameter& parameter) {
  if ( !parameter.expression ) {
    return std::nullopt;
  }

  if ( auto attribute = parameter.expression->isAttribute() ) {
    return attribute->type;
  }

  auto& root = parameter.expression->compiled.getRoot();
  auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);
  auto value = BPMNOS::trim_copy( element.value->get().value );

  if ( node.type == LIMEX::Type::literal && value.starts_with('"') ) {
    return BPMNOS::STRING;
  }

  throw std::runtime_error("MessageDefinition: header parameter '" + parameter.name + "' must state an attribute or a quoted string");
}

} // namespace

MessageDefinition::MessageDefinition(XML::bpmnos::tMessage* message, const AttributeRegistry& attributeRegistry)
  : element(message)
  , name( BPMNOS::to_number(message->name.value.value,STRING) )
{
  header.resize(3);
  // these hold the message name and the instance identifiers of the sending and the receiving token
  header[ Index::Name ] = { "name", STRING };
  header[ Index::Sender ] = { "sender", STRING };
  header[ Index::Recipient ] = { "recipient", STRING };

  std::map< std::string, std::optional<ValueType> > additionalHeader;
  for ( XML::bpmnos::tParameter& parameter : element->parameter ) {
    auto& key = parameter.name.value.value;
    auto& inserted = parameterMap.emplace(key,std::make_unique<Parameter>(&parameter,attributeRegistry)).first->second;
    auto type = getHeaderType(parameter,*inserted);

    if ( key == "name" || key == "sender" || key == "recipient" ) {
      if ( type.has_value() && type.value() != STRING ) {
        throw std::runtime_error("MessageDefinition: header parameter '" + key + "' must state a string");
      }
    }
    else {
      additionalHeader.emplace(key,type);
    }
  }
  for ( auto& [key,type] : additionalHeader ) {
    header.push_back({key,type});
  }

  for ( XML::bpmnos::tContent& content : element->content ) {
    contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeRegistry));
  }
}

template <typename DataType>
BPMNOS::Values MessageDefinition::getSenderHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::Values headerValues;

  for ( auto& [key,type] : header ) {
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

  for ( auto& [key,type] : header ) {
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

