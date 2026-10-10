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
      if ( input->category == Attribute::Category::STATUS && !input->isObject() && input->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        throw std::runtime_error("Conditions: condition '" + (std::string)condition.id.value + "' is time dependent");
      }
      dataDependencies.insert(input);
    }
  }  
}

template <typename DataType>
bool Conditions::conditionsSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& condition : conditions ) {
    if ( !condition->isSatisfied(status,data) ) {
      return false; 
    }
  }
  return true; 
}

template bool Conditions::conditionsSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool Conditions::conditionsSatisfied<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
