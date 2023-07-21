#include "Status.h"
#include "model/parser/src/xml/bpmnos/tStatus.h"
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/parser/src/xml/bpmnos/tRestrictions.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "model/parser/src/xml/bpmnos/tOperators.h"
#include "model/parser/src/xml/bpmnos/tOperator.h"

using namespace BPMNOS;

Status::Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement )
  , parent(parent)
{
  parentSize = parent ? parent->extensionElements->as<Status>()->size() : 0;
  if ( parent ) {
    attributeMap = parent->extensionElements->as<Status>()->attributeMap;
  }

  for ( XML::bpmnos::tAttribute& attribute : get<XML::bpmnos::tStatus,XML::bpmnos::tAttribute>() ) {
    attributes.push_back(std::make_unique<Attribute>(&attribute,attributeMap));
    attributeMap[attribute.name.value] = attributes.rbegin()->get();
  }

  for ( XML::bpmnos::tRestriction& restriction : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    try {
      restrictions.push_back( std::make_unique<Restriction>( &restriction,  attributeMap ) );
    }
    catch ( ... ){
      throw std::runtime_error("Status: illegal parameters for restriction '" + (std::string)restriction.id + "'");
    }
  }

  for ( XML::bpmnos::tOperator& operator_ : get<XML::bpmnos::tOperators,XML::bpmnos::tOperator>() ) {
    try {
      operators.push_back( std::make_unique<Operator>( &operator_,  attributeMap ) );
    }
    catch ( ... ){
      throw std::runtime_error("Status: illegal parameters for operator '" + (std::string)operator_.id + "'");
    }
  }
}

