#include "Restriction.h"
#include "model/parser/src/xml/bpmnos/tEnumeration.h"

using namespace BPMNOS::Model;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap)
  : element(restriction)
  , id(restriction->id.value.value)
{
  if ( !attributeMap.contains(restriction->attribute.value) ) {
    throw std::runtime_error("Restriction: unknown attribute '" + (std::string)restriction->attribute.value + "' for restriction '" + id + "'");
  } 
  attribute = attributeMap.at(restriction->attribute.value);

  if ( restriction->negate.has_value() ) {
    negated = (bool)restriction->negate->get().value;
  }
  else {
    negated = false;
  }

  if ( restriction->required.has_value() ) {
    required = (bool)restriction->required->get().value;
  }
  else {
    required = false;
  }

  if ( restriction->minInclusive.has_value() ) {
    if ( attribute->type == ValueType::STRING ) {
      throw std::runtime_error("Restriction: minInclusive not allowed attribute type 'xs:string' for restriction '" + id + "'");
    }
    minInclusive = to_number( restriction->minInclusive->get().getRequiredAttributeByName("value").value, attribute->type );
  }
  else {
    minInclusive = std::nullopt;
  }

  if ( restriction->maxInclusive.has_value() ) {
    if ( attribute->type == ValueType::STRING ) {
      throw std::runtime_error("Restriction: minInclusive not allowed attribute type 'string' for restriction '" + id + "'");
    }
    maxInclusive = to_number( restriction->maxInclusive->get().getRequiredAttributeByName("value").value, attribute->type );
  }
  else {
    maxInclusive = std::nullopt;
  }

  for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
    enumeration.insert( to_number(allowedValue.getRequiredAttributeByName("value").value, attribute->type) );
  }
}


bool Restriction::isSatisfied(const Values& status) const {
  if ( !negated ) {
    // if restriction is not negated
    if ( required && !status[attribute->index].has_value() ) {
      return false;
    }

    if ( minInclusive.has_value() && status[attribute->index].has_value() ) {
      if ( status[attribute->index].value() < minInclusive.value() ) {
        return false;
      }
    }

    if ( maxInclusive.has_value() && status[attribute->index].has_value() ) {
      if ( status[attribute->index].value() > maxInclusive.value() ) {
        return false;
      }
    }

    if ( enumeration.size() && status[attribute->index].has_value() ) {
      if ( enumeration.find( status[attribute->index].value() ) == enumeration.end() ) {
        return false;
      }
    }
  }
  else {
    // if restriction is negated
    if ( required && status[attribute->index].has_value() ) {
      return false;
    }

    if ( minInclusive.has_value() && status[attribute->index].has_value() ) {
      if ( status[attribute->index].value() >= minInclusive.value() ) {
        return false;
      }
    }

    if ( maxInclusive.has_value() && status[attribute->index].has_value() ) {
      if ( status[attribute->index].value() <= maxInclusive.value() ) {
        return false;
      }
    }

    if ( enumeration.size() && status[attribute->index].has_value() ) {
      if ( enumeration.find( status[attribute->index].value() ) != enumeration.end() ) {
        return false;
      }
    }
  }
  return true;
}  


