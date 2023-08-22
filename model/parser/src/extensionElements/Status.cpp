#include "Status.h"
#include "model/parser/src/xml/bpmnos/tStatus.h"
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/parser/src/xml/bpmnos/tRestrictions.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "model/parser/src/xml/bpmnos/tOperators.h"
#include "model/parser/src/xml/bpmnos/tOperator.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS;

Status::Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement )
  , parent(parent)
{
  parentSize = parent ? parent->extensionElements->as<Status>()->size() : 0;
  if ( parent ) {
    attributeMap = parent->extensionElements->as<Status>()->attributeMap;
  }

  if ( !element ) return; 

  if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
    // add all attributes
    if ( status->get().attributes.has_value() ) {
      for ( XML::bpmnos::tAttribute& attribute : status->get().attributes.value().get().attribute ) {
        attributes.push_back(std::make_unique<Attribute>(&attribute,attributeMap));
        attributeMap[attribute.name.value.value] = attributes.rbegin()->get();
      }
    }    
    // add all restrictions
    if ( status->get().restrictions.has_value() ) {
      for ( XML::bpmnos::tRestriction& restriction : status->get().restrictions.value().get().restriction ) {
        try {
          restrictions.push_back(std::make_unique<Restriction>(&restriction,attributeMap));
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal parameters for restriction '" + (std::string)restriction.id.value + "'");
        }
      }
    }    
    // add all operators
    if ( status->get().operators.has_value() ) {
      for ( XML::bpmnos::tOperator& operator_ : status->get().operators.value().get().operator_ ) {
        try {
          operators.push_back(Operator::create(&operator_,attributeMap));
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal parameters for operator '" + (std::string)operator_.id.value + "'");
        }
      }
    }    
    // add all decisions
    if ( status->get().decisions.has_value() ) {
      for ( XML::bpmnos::tDecision& decision : status->get().decisions.value().get().decision ) {
        try {
          decisions.push_back(std::make_unique<Decision>(&decision,attributeMap));
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal attributes for decision '" + (std::string)decision.id.value + "'");
        }
      }
    }    
  }
}

bool Status::isFeasible(const Values& values) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(values) ) {
      return false; 
    }
  }
  return true; 
}  

void Status::applyOperators(Values& values) const {
  for ( auto& operator_ : operators ) {
    operator_->apply(values);
  }
}

void Status::makeChoices(const std::unordered_map<Decision*,number>& choices, Values& values) const {
  for ( auto& [decision,value] : choices ) {
    values[decision->attribute->index] = value;
  }
}

