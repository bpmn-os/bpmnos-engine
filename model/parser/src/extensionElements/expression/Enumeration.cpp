#include "Enumeration.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/VectorRegistry.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

Enumeration::Enumeration(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap)
  : Expression(parameter, attributeMap)
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

  if ( auto it = attributeMap.find(attributeName);
    it != attributeMap.end()
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
    
  // get enumeration entries
  if ( strutil::starts_with(trimmed,"[") && strutil::ends_with(trimmed,"]") ) {
    // determine comma separated values    
    auto elements = strutil::split( trimmed.substr(1,trimmed.size()-2), "," );
    collection = std::vector< std::variant<const Attribute*, std::optional<BPMNOS::number> > >();
    auto& entries = std::get< std::vector< std::variant<const Attribute*, std::optional<BPMNOS::number> > > >(collection); 
    for ( auto& element : elements ) {
      strutil::trim(element);
      if ( strutil::starts_with(element,"\"") && strutil::ends_with(element,"\"") ) {
        entries.push_back( BPMNOS::to_number( element.substr(1,element.size()-2), STRING ) );
      }
      else if ( element == Keyword::False ) {
        entries.push_back( BPMNOS::to_number( false , BOOLEAN) );
      }
      else if ( element == Keyword::True ) {
        entries.push_back( BPMNOS::to_number( true , BOOLEAN) );
      }
      else if ( element == Keyword::Undefined ) {
        entries.push_back( std::nullopt );
      }
      else {
        if ( auto it = attributeMap.find(element);
          it != attributeMap.end()
        ) {
          entries.push_back( it->second );
        }
        else {
          throw std::runtime_error("Enumeration: cannot find attribute of enumeration entry");
        }
      }
    }
  }
  else {
    if ( auto it = attributeMap.find(trimmed);
      it != attributeMap.end()
    ) {
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
      auto& entries = vectorRegistry[(long unsigned int)values[other->index].value()];
      for ( auto entry : entries ) {
        if ( values[attribute->index] == entry ) {
          found = true;
          break;
        }
      }      
    }
  }
  else {
    auto& entries = std::get< std::vector< std::variant<const Attribute*, std::optional<BPMNOS::number> > > >(collection);
    for ( auto entry : entries ) {
      if ( std::holds_alternative<const Attribute *>(entry) ) {
        auto other = std::get<const Attribute *>(entry);
        if ( values[attribute->index] == values[other->index] ) {
          found = true;
          break;
        }
      }
      else {
        if ( values[attribute->index] == std::get< std::optional<BPMNOS::number> >(entry) ) {
          found = true;
          break;
        }
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

