#include "Attribute.h"

using namespace BPMNOS;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap)
  : element(attribute)
  , index(attributeMap.size())
  , id(attribute->id.value.value)
  , name(attribute->name.value.value)
{
  if ( attribute->type.value.value == "xs:string" ) {
    type = Type::STRING;
    if ( attribute->value.has_value() ) {
      value = (std::string)attribute->value->get().value;
    }
  }
  else if ( attribute->type.value.value == "xs:boolean" ) {
    type = Type::BOOLEAN;
    if ( attribute->value.has_value() ) {
      value = (bool)attribute->value->get().value;
    }
  }
  else if ( attribute->type.value.value == "xs:integer" ) {
    type = Type::INTEGER;
    if ( attribute->value.has_value() ) {
      value = (int)attribute->value->get().value;
    }
  }
  else if ( attribute->type.value.value == "xs:decimal" ) {
    type = Type::DECIMAL;
    if ( attribute->value.has_value() ) {
      value = (double)attribute->value->get().value;
    }
  }



  if ( attribute->weight.has_value() ) {
    if ( attribute->objective.has_value() && attribute->objective->get().value.value == "maximize" ) {
      weight = -(double)attribute->weight->get().value;
    }
    else if ( attribute->objective.has_value() &&  attribute->objective->get().value.value == "minimize" ) {
      weight = (double)attribute->weight->get().value;
    }
    else {
      throw std::runtime_error("Attribute: illegal objective of attribute '" + id + "'");
    }
  }
  else {
    weight = 0;
  }
}
