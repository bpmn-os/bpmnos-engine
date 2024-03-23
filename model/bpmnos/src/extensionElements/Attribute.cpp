#include "Attribute.h"
#include "model/utility/src/Keywords.h"
#include "Parameter.h"

using namespace BPMNOS::Model;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, AttributeMap& attributeMap)
  : element(attribute)
  , index(attributeMap.size())
  , id(attribute->id.value.value)
  , name(attribute->name.value.value)
{
  if ( attribute->type.value.value == "boolean" ) {
    type = ValueType::BOOLEAN;
  }
  else if ( attribute->type.value.value == "integer" ) {
    type = ValueType::INTEGER;
  }
  else if ( attribute->type.value.value == "decimal" ) {
    type = ValueType::DECIMAL;
  }
  else if ( attribute->type.value.value == "string" ) {
    type = ValueType::STRING;
  }
  else if ( attribute->type.value.value == "collection" ) {
    type = ValueType::COLLECTION;
  }

  if ( attribute->parameter.has_value() ) {
    auto& parameter = attribute->parameter.value().get();
    if ( parameter.name.value.value == "collection" ) {
      collection = std::make_unique<Parameter>(&parameter,attributeMap);
    }
    else {
      throw std::runtime_error("Attribute: illegal parameter provided for attribute '" + id + "'");
    }
  }

  if ( attribute->value.has_value() ) {
    value = to_number( attribute->value->get().value.value, type ); 
  }

  if ( attribute->weight.has_value() ) {
    if ( attribute->objective.has_value() && attribute->objective->get().value.value == "maximize" ) {
      weight = (double)attribute->weight->get().value;
    }
    else if ( attribute->objective.has_value() &&  attribute->objective->get().value.value == "minimize" ) {
      weight = -(double)attribute->weight->get().value;
    }
    else {
      throw std::runtime_error("Attribute: illegal objective of attribute '" + id + "'");
    }
  }
  else {
    weight = 0;
  }

  isImmutable = (id != Keyword::Timestamp);
}
