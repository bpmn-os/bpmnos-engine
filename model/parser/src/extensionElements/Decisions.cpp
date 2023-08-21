#include "Decisions.h"
#include "Status.h"
#include "model/parser/src/xml/bpmnos/tDecisions.h"
#include "model/parser/src/xml/bpmnos/tDecision.h"

using namespace BPMNOS;

Decisions::Decisions(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : Status( baseElement, parent ) 
{
  for ( XML::bpmnos::tDecision& decision : get<XML::bpmnos::tDecisions, XML::bpmnos::tDecision>() ) {
    decisions.push_back(std::make_unique<Decision>(&decision,attributeMap));
  }
}

void Decisions::makeChoices(const std::unordered_map<Decision*,number>& choices, Values& values) const {
  for ( auto& [decision,value] : choices ) {
    values[decision->attribute->index] = value;
  }
}

