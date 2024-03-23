#include "Guidance.h"
#include "model/data/src/Scenario.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "model/bpmnos/src/xml/bpmnos/tOperators.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Model;

Guidance::Guidance(XML::bpmnos::tGuidance* guidance,  AttributeMap attributeMap)
  : element(guidance)
  , attributeMap(attributeMap)
{
  if ( guidance->type.value.value == "message" ) {
    type = Type::MessageDelivery;
  }
  else if ( guidance->type.value.value == "entry" ) {
    type = Type::Entry;
  }
  else if ( guidance->type.value.value == "exit" ) {
    type = Type::Exit;
  }
  else if ( guidance->type.value.value == "choice" ) {
    type = Type::Choice;
  }

  // add all attributes
  if ( guidance->attributes.has_value() ) {
    for ( XML::bpmnos::tAttribute& attributeElement : guidance->attributes.value().get().attribute ) {
      auto attribute = std::make_unique<Attribute>(&attributeElement,attributeMap);
      if ( attributeMap.contains(attribute->name) ) {
        throw std::runtime_error("Guidance: illegal redeclaration of attribute '" + (std::string)attribute->name + "'");
      }
      else { 
        attributeMap[attribute->name] = attribute.get();
        attributes.push_back(std::move(attribute));
      }
    }
  }    
  // add all restrictions
  if ( guidance->restrictions.has_value() ) {
    for ( XML::bpmnos::tRestriction& restriction : guidance->restrictions.value().get().restriction ) {
      try {
        restrictions.push_back(std::make_unique<Restriction>(&restriction,attributeMap));
      }
      catch ( ... ){
        throw std::runtime_error("Guidance: illegal parameters for restriction '" + (std::string)restriction.id.value + "'");
      }
    }
  }    
  // add all operators
  if ( guidance->operators.has_value() ) {
    for ( XML::bpmnos::tOperator& operator_ : guidance->operators.value().get().operator_ ) {
      try {
        operators.push_back(Operator::create(&operator_,attributeMap));
      }
      catch ( ... ){
        throw std::runtime_error("Guidance: illegal parameters for operator '" + (std::string)operator_.id.value + "'");
      }
    }
  }    
}

BPMNOS::number Guidance::getObjective(const Values& values) const {
  BPMNOS::number objective = 0;
  for ( auto& [name, attribute] : attributeMap ) {
    if ( attribute->index >= values.size() ) {
      break;
    }
    if ( values[attribute->index].has_value() ) {
      objective += attribute->weight * values[attribute->index].value();
    }
  }
  return objective;
}

bool Guidance::restrictionsSatisfied(const Values& values) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(values) ) {
      return false;
    }
  }
  
  // TODO: check node restrictions
  return true;
}


std::optional<BPMNOS::number> Guidance::apply(Values& status, const BPMN::FlowNode* node, const Scenario* scenario, BPMNOS::number currentTime) const {
  Values guidingAttributes;
  auto values = scenario->getKnownValues(node, status, currentTime );
  if ( values ) {
    guidingAttributes = std::move( values.value() );
  }
  else {
    throw std::runtime_error("Guidance: guiding attributes are not known");
  }

  // add new attributes
  status.insert(status.end(), guidingAttributes.begin(), guidingAttributes.end());

  // apply operators
  for ( auto& operator_ : operators ) {
    operator_->apply(status);
  }
  
  // check feasibility
  if ( restrictionsSatisfied(status) ) {
    // return guiding value
    return getObjective(status);
  }
  
  return std::nullopt;
}

