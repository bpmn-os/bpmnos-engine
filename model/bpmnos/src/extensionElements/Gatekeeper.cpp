#include "Gatekeeper.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"

using namespace BPMNOS::Model;

Gatekeeper::Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeRegistry& attributeRegistry = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  for ( XML::bpmnos::tRestriction& condition : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    try {
      conditions.push_back(std::make_unique<Restriction>(&condition,attributeRegistry));
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("Gatekeeper: illegal parameters for condition '" + (std::string)condition.id.value + "'.\n" + error.what());
    }
  }
}

template <typename DataType>
bool Gatekeeper::conditionsSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& condition : conditions ) {
    if ( !condition->isSatisfied(status,data) ) {
      return false; 
    }
  }
  return true; 
}

template bool Gatekeeper::conditionsSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool Gatekeeper::conditionsSatisfied<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
