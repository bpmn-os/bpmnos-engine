#include "Restriction.h"
#include "Status.h"
#include "xml/bpmnos/tEnumeration.h"

using namespace BPMNOS;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap)
  : element(restriction)
  , id(restriction->id.value)
  , attribute(attributeMap.at(restriction->attribute))
{
  if ( restriction->negate.has_value() ) {
    negated = (bool)restriction->negate->get(); 
  }
  else {
    negated = false;
  }

  if ( restriction->required.has_value() ) {
    required = (bool)restriction->required->get(); 
  }
  else {
    required = false;
  }

  if ( restriction->minInclusive.has_value() ) {
    minInclusive = (double)restriction->minInclusive->get(); 
  }
  else {
    minInclusive = std::nullopt;
  }

  if ( restriction->maxInclusive.has_value() ) {
    maxInclusive = (double)restriction->maxInclusive->get(); 
  }
  else {
    maxInclusive = std::nullopt;
  }

  switch ( attribute->type ) {
    case Attribute::Type::STRING :
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< std::string > >(this->enumeration).push_back(
          allowedValue.getRequiredAttributeByName("value").value
        );
      }
      break;
    case Attribute::Type::BOOLEAN :
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< bool > >(this->enumeration).push_back(
          (bool)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
    case Attribute::Type::INTEGER :
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< int > >(this->enumeration).push_back(
          (int)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
    case Attribute::Type::DECIMAL :
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< double > >(this->enumeration).push_back(
          (double)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
  }
}

