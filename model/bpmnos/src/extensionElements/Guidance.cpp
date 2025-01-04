#include "Guidance.h"
#include "model/data/src/Scenario.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "model/bpmnos/src/xml/bpmnos/tOperators.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "model/utility/src/Keywords.h"
//#include <iostream>

using namespace BPMNOS::Model;

Guidance::Guidance(XML::bpmnos::tGuidance* guidance, const AttributeRegistry& attributeRegistry)
  : element(guidance)
  , attributeRegistry(attributeRegistry)
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
      auto attribute = std::make_unique<Attribute>(&attributeElement, Attribute::Category::STATUS, this->attributeRegistry);
      attributes.push_back(std::move(attribute));
    }
  }    
  // add all restrictions
  if ( guidance->restrictions.has_value() ) {
    for ( XML::bpmnos::tRestriction& restriction : guidance->restrictions.value().get().restriction ) {
      try {
        restrictions.push_back(std::make_unique<Restriction>(&restriction,this->attributeRegistry));
        for ( auto input : restrictions.back()->expression.inputs ) {
          dependencies.insert(input);
        }
      }
      catch ( const std::exception &error ){
        throw std::runtime_error("Guidance: illegal expression for restriction '" + (std::string)restriction.id.value + "'\n" + error.what() );
      }
    }
  }    
  // add all operators
  if ( guidance->operators.has_value() ) {
    for ( XML::bpmnos::tOperator& operator_ : guidance->operators.value().get().operator_ ) {
      try {
        operators.push_back(std::make_unique<Operator>(&operator_,this->attributeRegistry));
        for ( auto input : operators.back()->expression.inputs ) {
          dependencies.insert(input);
        }
      }
      catch ( const std::exception &error ){
        throw std::runtime_error("Guidance: illegal expression for operator '" + (std::string)operator_.id.value + "'" + "'\n" + error.what() );
      }
    }
  }    
}

template <typename DataType>
BPMNOS::number Guidance::getObjective(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::number objective = 0;
  for ( auto& [name, attribute] : attributeRegistry.statusAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto& [name, attribute] : attributeRegistry.dataAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto& [name, attribute] : attributeRegistry.globalAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  return objective;
}

template BPMNOS::number Guidance::getObjective<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
//template BPMNOS::number Guidance::getObjective<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

template <typename DataType>
bool Guidance::restrictionsSatisfied(const BPMN::FlowNode* node, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(status,data,globals) ) {
      return false;
    }
  }
  
  // TODO: do we need to check node restrictions?
  return true;
}

template bool Guidance::restrictionsSatisfied<BPMNOS::Values>(const BPMN::FlowNode* node, const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
//template bool Guidance::restrictionsSatisfied<BPMNOS::SharedValues>(const BPMN::FlowNode* node, const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


template <typename DataType>
void Guidance::apply(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {

  for ( auto& attribute : attributes ) {
    status.push_back( scenario->getKnownValue(instanceId, attribute.get(), currentTime ) );
  }

  // apply operators
  for ( auto& operator_ : operators ) {
    operator_->apply(status,data,globals);
  }
}

template void Guidance::apply<BPMNOS::Values>(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
//template void Guidance::apply<BPMNOS::SharedValues>(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;

