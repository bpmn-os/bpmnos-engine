#include "Parameter.h"
#include <strutil.h>
#include "model/utility/src/encode_quoted_strings.h"
#include "model/utility/src/encode_collection.h"

using namespace BPMNOS::Model;

Parameter::Parameter(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : element(parameter)
  , name(parameter->name.value.value)
  , expression(getExpression(parameter, attributeRegistry))
{
}

std::unique_ptr<const Expression> Parameter::getExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry) const {
  if ( !parameter->value.has_value() || strutil::trim_copy(parameter->value->get().value).empty() ) {
    return nullptr;
  }

  return std::make_unique<const Expression>(encodeCollection(encodeQuotedStrings(parameter->value->get().value)),attributeRegistry,true);
}

