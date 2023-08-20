#include "Attribute.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap)
  : element(attribute)
  , index(attributeMap.size())
  , id(attribute->id.value.value)
  , name(attribute->name.value.value)
{
  if ( attribute->type.value.value == "xs:string" ) {
    type = ValueType::STRING;
  }
  else if ( attribute->type.value.value == "xs:boolean" ) {
    type = ValueType::BOOLEAN;
  }
  else if ( attribute->type.value.value == "xs:integer" ) {
    type = ValueType::INTEGER;
  }
  else if ( attribute->type.value.value == "xs:decimal" ) {
    type = ValueType::DECIMAL;
  }

  if ( attribute->value.has_value() ) {
    value = to_number( attribute->value->get().value.value, type ); 
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

  isImmutable = (name != Keyword::Timestamp);
}
