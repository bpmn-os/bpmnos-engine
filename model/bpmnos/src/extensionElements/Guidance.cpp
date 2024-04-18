#include "Guidance.h"
#include "model/data/src/Scenario.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "model/bpmnos/src/xml/bpmnos/tOperators.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "model/utility/src/Keywords.h"

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
        for ( auto input : restrictions.back()->expression->inputs ) {
          dependencies.insert(input);
        }
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
        operators.push_back(Operator::create(&operator_,this->attributeRegistry));
        for ( auto input : operators.back()->inputs ) {
          dependencies.insert(input);
        }
      }
      catch ( ... ){
        throw std::runtime_error("Guidance: illegal parameters for operator '" + (std::string)operator_.id.value + "'");
      }
    }
  }    
}

template <typename DataType>
BPMNOS::number Guidance::getObjective(const BPMNOS::Values& status, const DataType& data) const {
  BPMNOS::number objective = 0;
  for ( auto& [name, attribute] : attributeRegistry.statusAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() ) {
      objective += attribute->weight * value.value();
    }
  }
  for ( auto& [name, attribute] : attributeRegistry.dataAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() ) {
      objective += attribute->weight * value.value();
    }
  }
  return objective;
}

template BPMNOS::number Guidance::getObjective<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data) const;
//template BPMNOS::number Guidance::getObjective<BPMNOS::Globals>(const BPMNOS::Values& status, const BPMNOS::Globals& data) const;

template <typename DataType>
bool Guidance::restrictionsSatisfied(const BPMN::FlowNode* node, const BPMNOS::Values& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(status,data) ) {
      return false;
    }
  }
  
  // TODO: do we need to check node restrictions?
  return true;
}

template bool Guidance::restrictionsSatisfied<BPMNOS::Values>(const BPMN::FlowNode* node, const BPMNOS::Values& status, const BPMNOS::Values& data) const;
//template bool Guidance::restrictionsSatisfied<BPMNOS::Globals>(const BPMN::FlowNode* node, const BPMNOS::Values& status, const BPMNOS::Globals& data) const;


template <typename DataType>
void Guidance::apply(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, DataType& data) const {

  for ( auto& attribute : attributes ) {
    status.push_back( scenario->getKnownValue(instanceId, attribute.get(), currentTime ) );
  }

  // apply operators
  for ( auto& operator_ : operators ) {
    operator_->apply(status,data);
  }
}

template void Guidance::apply<BPMNOS::Values>(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, BPMNOS::Values& data) const;
//template void Guidance::apply<BPMNOS::Globals>(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, BPMNOS::Globals& data) const;

