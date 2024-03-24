#include "Enumeration.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/CollectionRegistry.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

Enumeration::Enumeration(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes)
  : Expression(parameter, statusAttributes)
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

  if ( auto it = statusAttributes.find(attributeName);
    it != statusAttributes.end()
  ) {
    attribute = it->second;
  }
  else {
    throw std::runtime_error("Enumeration: cannot find attribute");
  }

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
    if ( auto it = statusAttributes.find(trimmed);
      it != statusAttributes.end()
    ) {
      if ( it->second->type != ValueType::COLLECTION ) {
        throw std::runtime_error("Enumeration: variable '" + attribute->name + "' is not a collection");
      }
      collection = it->second;
    }
    else {
      throw std::runtime_error("Enumeration: cannot determine collection");
    }
  } 
}

std::optional<BPMNOS::number> Enumeration::execute(const Values& values) const {
  bool found = false;
  if ( std::holds_alternative<const Attribute *>(collection) ) {
    auto other = std::get<const Attribute *>(collection);
    if ( values[other->index].has_value() ) {
      auto& entries = collectionRegistry[(long unsigned int)values[other->index].value()].values;
      for ( auto entry : entries ) {
        if ( values[attribute->index] == entry ) {
          found = true;
          break;
        }
      }      
    }
  }
  else {
    for ( auto value : collectionRegistry[(long unsigned int)std::get< BPMNOS::number >(collection)].values ) {
      if ( values[attribute->index] == value ) {
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

