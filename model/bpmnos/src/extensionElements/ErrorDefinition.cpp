#include "ErrorDefinition.h"
#include <stdexcept>

using namespace BPMNOS::Model;

ErrorDefinition::ErrorDefinition(XML::bpmnos::tError* error)
  : element(error)
  , name( BPMNOS::to_number(error->name.value.value,STRING) )
{
  // the content is an object, without fields if the error states no type
  if ( error->type.has_value() ) {
    schema = Schema::parse(error->type->get().value.value);
    if ( std::holds_alternative<ValueType>(schema) ) {
      throw std::runtime_error("ErrorDefinition: the type of error '" + error->name.value.value + "' must be an object");
    }
  }
}
