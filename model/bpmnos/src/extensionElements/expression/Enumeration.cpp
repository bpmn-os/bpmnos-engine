#include "Enumeration.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/CollectionRegistry.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

Enumeration::Enumeration(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : Expression(parameter, attributeRegistry)
{
  if ( !parameter->value.has_value() ) {
    throw std::runtime_error("Enumeration: list of allowed values must be provided");
  }

  // get first word which must refer to the attribute name
  std::string trimmed = strutil::trim_copy(expression);

  auto pos = trimmed.find(" ");    
  if ( pos == std::string::npos) {
    throw std::runtime_error("Enumeration: cannot identify attribute name");
  }
  
  auto attributeName = trimmed.substr(0,pos);

  attribute = attributeRegistry[attributeName];

  trimmed = strutil::trim_copy( trimmed.substr(pos) );

  // get type
  if ( strutil::starts_with(trimmed,IN) ) {
    type = Type::IN;
    trimmed = strutil::trim_copy( trimmed.substr(IN.size()) );
  }
  else if ( strutil::starts_with(trimmed,NOTIN) ) {
    type = Type::NOTIN;
    trimmed = strutil::trim_copy( trimmed.substr(NOTIN.size()) );
  } 
  else {
    throw std::runtime_error("Enumeration: illegal comparison");
  }
    
  if ( trimmed.empty() ) {
    throw std::runtime_error("Enumeration: cannot find collection entries");
  }

  // get enumeration entries
  if ( trimmed[0] == '[') {
    collection = collectionRegistry(trimmed);
  }
  else {
    collection = attributeRegistry[trimmed];
  } 
}

template <typename DataType>
std::optional<BPMNOS::number> Enumeration::_execute(const BPMNOS::Values& status, const DataType& data) const {
  bool found = false;
  auto attributeValue = attributeRegistry.getValue(attribute,status,data);
  if ( std::holds_alternative<const Attribute *>(collection) ) {
    auto other = std::get<const Attribute *>(collection);
    auto otherValue = attributeRegistry.getValue(other,status,data);
    if ( otherValue.has_value() ) {
      auto& entries = collectionRegistry[(long unsigned int)otherValue.value()].values;
      for ( auto entry : entries ) {
        if ( attributeValue == entry ) {
          found = true;
          break;
        }
      }      
    }
  }
  else {
    for ( auto value : collectionRegistry[(long unsigned int)std::get< BPMNOS::number >(collection)].values ) {
      if ( attributeValue == value ) {
        found = true;
        break;
      }
    }
  }

  if ( type == Type::IN ) {
    return BPMNOS::to_number( found , BOOLEAN);
  }
  else if ( type == Type::NOTIN ) {
    return BPMNOS::to_number( !found , BOOLEAN);
  } 
  return std::nullopt;
}

template std::optional<BPMNOS::number> Enumeration::_execute<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data) const;
template std::optional<BPMNOS::number> Enumeration::_execute<BPMNOS::Globals>(const BPMNOS::Values& status, const BPMNOS::Globals& data) const;

