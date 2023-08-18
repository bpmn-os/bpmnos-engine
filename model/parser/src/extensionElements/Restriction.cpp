#include "Restriction.h"
#include "Status.h"
#include "model/parser/src/xml/bpmnos/tEnumeration.h"

using namespace BPMNOS;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap)
  : element(restriction)
  , id(restriction->id.value)
{
  if ( !attributeMap.contains(restriction->attribute) ) {
    throw std::runtime_error("Restriction: unknown attribute '" + (std::string)restriction->attribute + "' for restriction '" + (std::string)restriction->id + "'");
  } 
  attribute = attributeMap.at(restriction->attribute);

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
      enumeration = std::vector< std::string >();
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< std::string > >(enumeration).push_back(
          allowedValue.getRequiredAttributeByName("value").value
        );
      }
      break;
    case Attribute::Type::BOOLEAN :
      enumeration = std::vector< bool >();
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< Attribute::Type::BOOLEAN >(enumeration).push_back( 
          (bool)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
    case Attribute::Type::INTEGER :
      enumeration = std::vector< int >();
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< int > >(enumeration).push_back(
          (int)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
    case Attribute::Type::DECIMAL :
      enumeration = std::vector< double >();
      for ( XML::bpmnos::tEnumeration& allowedValue : restriction->enumeration ) {
        std::get< std::vector< double > >(enumeration).push_back(
          (double)allowedValue.getRequiredAttributeByName("value")
        );
      }
      break;
  }

}

