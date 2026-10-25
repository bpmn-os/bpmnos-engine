#include "MessageDefinition.h"
#include "ExtensionElements.h"
#include "model/utility/src/string_utility.h"
#include <map>
#include <stdexcept>

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
    if ( attribute->isObject() ) {
      throw std::runtime_error("MessageDefinition: header parameter '" + parameter.name + "' must not state an object");
    }
    return std::get<BPMNOS::ValueType>(attribute->type);
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
  // the content is an object, without fields if the message states no type
  if ( message->type.has_value() ) {
    schema = Schema::parse(message->type->get().value.value);
    if ( std::holds_alternative<ValueType>(schema) ) {
      throw std::runtime_error("MessageDefinition: the type of message '" + message->name.value.value + "' must be an object");
    }
  }

  header.resize(2);
  // these hold the instance identifiers of the sending and the receiving token
  header[ Index::Sender ] = { "sender", STRING };
  header[ Index::Recipient ] = { "recipient", STRING };

  std::map< std::string, std::optional<ValueType> > additionalHeader;
  for ( XML::bpmnos::tParameter& parameter : element->parameter ) {
    auto& key = parameter.name.value.value;
    auto& inserted = parameterMap.emplace(key,std::make_unique<Parameter>(&parameter,attributeRegistry)).first->second;
    auto type = getHeaderType(parameter,*inserted);

    if ( key == "sender" || key == "recipient" ) {
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
}

template <typename DataType>
BPMNOS::Values MessageDefinition::getSenderHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const DataType& data, BPMNOS::number instance) const {
  BPMNOS::Values headerValues;
  headerValues.reserve(header.size());

  for ( auto& [key,type] : header ) {
    if ( key == "sender" ) {
      headerValues.push_back( instance );
    }
    else {
      headerValues.push_back( getHeaderValue(key, attributeRegistry, status, data) );
    }
  }

  return headerValues;
}

template  BPMNOS::Values MessageDefinition::getSenderHeader<BPMNOS::Data>(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::Data& data, BPMNOS::number instance) const;
template  BPMNOS::Values MessageDefinition::getSenderHeader<BPMNOS::SharedData>(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::SharedData& data, BPMNOS::number instance) const;

template <typename DataType>
BPMNOS::Values MessageDefinition::getRecipientHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const DataType& data, BPMNOS::number instance) const {
  BPMNOS::Values headerValues;
  headerValues.reserve(header.size());

  for ( auto& [key,type] : header ) {
    if ( key == "recipient" ) {
      headerValues.push_back( instance );
    }
    else {
      headerValues.push_back( getHeaderValue(key, attributeRegistry, status, data) );
    }
  }

  return headerValues;
}

template  BPMNOS::Values MessageDefinition::getRecipientHeader<BPMNOS::Data>(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::Data& data, BPMNOS::number instance) const;
template  BPMNOS::Values MessageDefinition::getRecipientHeader<BPMNOS::SharedData>(const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::SharedData& data, BPMNOS::number instance) const;

template <typename DataType>
std::optional<BPMNOS::number> MessageDefinition::getHeaderValue(const std::string& key, [[maybe_unused]] const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const DataType& data) const {
  std::optional<BPMNOS::number> value;
  auto it = parameterMap.find(key);
  if ( it != parameterMap.end() && it->second->expression ) {
    value = BPMNOS::to_value( it->second->expression->execute(status,data) );
  }
  return value;
}

template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::Data>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::Data& data) const; 
template std::optional<BPMNOS::number> MessageDefinition::getHeaderValue<BPMNOS::SharedData>(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Status& status, const BPMNOS::SharedData& data) const; 

