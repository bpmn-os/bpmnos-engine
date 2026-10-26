#include "SignalDefinition.h"
#include <stdexcept>

using namespace BPMNOS::Model;

SignalDefinition::SignalDefinition(XML::bpmnos::tSignal* signal)
  : element(signal)
  , name( BPMNOS::to_number(signal->name.value.value,STRING) )
{
  // the content is a struct, without fields if the signal states no type
  if ( signal->type.has_value() ) {
    schema = Schema::parse(signal->type->get().value.value);
    if ( std::holds_alternative<ValueType>(schema) ) {
      throw std::runtime_error("SignalDefinition: the type of signal '" + signal->name.value.value + "' must be an object");
    }
  }
}
