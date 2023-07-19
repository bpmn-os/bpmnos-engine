#include "Attribute.h"

using namespace BPMNOS;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap)
  : element(attribute)
  , index(attributeMap.size())
  , id(attribute->id.value)
  , name(attribute->name.value)
{
  if ( attribute->type.value == "xs:string" ) {
    type = Type::STRING;
  }
  else if ( attribute->type.value == "xs:boolean" ) {
    type = Type::BOOLEAN;
  }
  else if ( attribute->type.value == "xs:integer" ) {
    type = Type::INTEGER;
  }
  else if ( attribute->type.value == "xs:decimal" ) {
    type = Type::DECIMAL;
  }

  if ( attribute->weight.has_value() ) {
    if ( attribute->objective->get().value == "maximize" ) {
      weight = -(double)attribute->weight->get();
    }
    else if ( (std::string_view)attribute->objective->get() == "minimize" ) {
      weight = (double)attribute->weight->get();
    }
    else {
      throw std::runtime_error("Attribute: illegal objective of attribute '" + id + "'");
    }
  }
  else {
    weight = 0;
  }
}
