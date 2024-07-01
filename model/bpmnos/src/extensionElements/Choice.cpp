#include "Choice.h"
#include "expression/LinearExpression.h"
#include "expression/Enumeration.h"
#include "model/utility/src/CollectionRegistry.h"
//#include <iostream>

using namespace BPMNOS::Model;

Choice::Choice(XML::bpmnos::tDecision* decision, const AttributeRegistry& attributeRegistry, const std::vector< std::unique_ptr<Restriction> >& restrictions)
  : element(decision)
  , attributeRegistry(attributeRegistry)
  , restrictions(restrictions)
  , attribute(attributeRegistry[element->attribute.value])
{
  attribute->isImmutable = false;

  if ( attribute->type == BOOLEAN ) {
    lowerBound = 0;
    upperBound = 1;
  }
  else {
    lowerBound = std::numeric_limits<BPMNOS::number>::lowest();
    upperBound = std::numeric_limits<BPMNOS::number>::max();
  }
}

std::pair<BPMNOS::number,BPMNOS::number> Choice::getBounds(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const {
  assert( attribute->type != STRING );
  
  BPMNOS::number min = lowerBound;
  BPMNOS::number max = upperBound;
  for ( auto& restriction : restrictions ) {
    if ( restriction->scope == BPMNOS::Model::Restriction::Scope::ENTRY ) {
      continue;
    }

    if ( auto linearExpression = dynamic_cast<BPMNOS::Model::LinearExpression*>(restriction->expression.get()) ) {
      // deduce stricter limits from linear restriction
      auto [lb,ub] = linearExpression->getBounds(attribute,status,data,globals);
//std::cout << "[" << (double)lb.value_or(-999) << "," << (double)ub.value_or(999) << "]" << std::endl;      
      if ( lb.has_value() && lb.value() > min ) {
         min = lb.value();
      }
      if ( ub.has_value() && ub.value() < max ) {
         max = ub.value();
      }
    }
  }
  
  if ( attribute->type != DECIMAL ) {
    min = std::ceil((double)min);
    max = std::floor((double)max);
  }
  
//std::cout << "=> [" << (double)min << "," << (double)max << "]" << std::endl;      
  return {min,max};
}

std::optional< std::vector<BPMNOS::number> > Choice::getEnumeration(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const {
  std::optional< std::vector<BPMNOS::number> > allowedValues;

  for ( auto& restriction : restrictions ) {
    if ( restriction->scope == BPMNOS::Model::Restriction::Scope::ENTRY ) {
      continue;
    }
    
    if ( auto enumeration = dynamic_cast<BPMNOS::Model::Enumeration*>(restriction->expression.get());
      enumeration && enumeration->type == BPMNOS::Model::Enumeration::Type::IN
    ) {
      // determine allowed values from enumeration
      if ( enumeration->attribute == attribute ) {
        auto collection = (
            std::holds_alternative<const BPMNOS::Model::Attribute*>(enumeration->collection) ?
            attributeRegistry.getValue(std::get<const BPMNOS::Model::Attribute *>(enumeration->collection),status,data,globals).value_or(0) :
            std::get<BPMNOS::number>(enumeration->collection)
        );

        if ( allowedValues.has_value() ) {
          throw std::runtime_error("Choice: redundant enumeration '" + restriction->id + "' provided for attribute '" + attribute->id + "'" );
        }
        else {
          std::vector<BPMNOS::number> values;
          auto& entries = collectionRegistry[(long unsigned int)collection].values;
          for ( auto entry : entries ) {
            values.push_back(entry.value());
          }
          allowedValues = std::move(values);
        }
      }
    }
  }
  return allowedValues;
}

