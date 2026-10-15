#include "EscalationDefinition.h"
#include <stdexcept>

using namespace BPMNOS::Model;

EscalationDefinition::EscalationDefinition(XML::bpmnos::tEscalation* escalation)
  : element(escalation)
  , name( BPMNOS::to_number(escalation->name.value.value,STRING) )
{
  // the content is an object, without fields if the escalation states no type
  if ( escalation->type.has_value() ) {
    schema = Schema::parse(escalation->type->get().value.value);
    if ( schema.isScalar() ) {
      throw std::runtime_error("EscalationDefinition: the type of escalation '" + escalation->name.value.value + "' must be an object");
    }
  }
}
