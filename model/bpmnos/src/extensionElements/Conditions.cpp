#include "Conditions.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"

using namespace BPMNOS::Model;

Conditions::Conditions(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeRegistry& attributeRegistry = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  for ( XML::bpmnos::tRestriction& condition : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    try {
      conditions.push_back(std::make_unique<Restriction>(&condition,attributeRegistry));
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("Conditions: illegal parameters for condition '" + (std::string)condition.id.value + "'.\n" + error.what());
    }
    
    // add data dependencies
    for ( auto input : conditions.back()->expression.inputs ) {
      if ( input->category == Attribute::Category::STATUS && input->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        throw std::runtime_error("Conditions: condition '" + (std::string)condition.id.value + "' is time dependent");
      }
      dataDependencies.insert(input);
    }
  }  
}

template <typename DataType>
bool Conditions::conditionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& condition : conditions ) {
    if ( !condition->isSatisfied(status,data,globals) ) {
      return false; 
    }
  }
  return true; 
}

template bool Conditions::conditionsSatisfied<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool Conditions::conditionsSatisfied<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;
