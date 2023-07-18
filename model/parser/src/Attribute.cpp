#include "Attribute.h"

using namespace BPMNOS;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, const AttributeMap& attributeMap)
  : element(attribute)
  , index(attributeMap.size())
  , id(attribute->id.value)
  , name(attribute->name.value)
{
  if ( (std::string)attribute->type == "xs:string" ) {
    type = Type::STRING;
  }
  else if ( (std::string)attribute->type == "xs:boolean" ) {
    type = Type::BOOLEAN;
  }
  else if ( (std::string)attribute->type == "xs:integer" ) {
    type = Type::INTEGER;
  }
  else if ( (std::string)attribute->type == "xs:decimal" ) {
    type = Type::DECIMAL;
  }

  if ( attribute->weight.has_value() ) {
    if ( (std::string)attribute->objective->get() == "maximize" ) {
      weight = -(double)attribute->weight->get();
    }
    else if ( (std::string)attribute->objective->get() == "minimize" ) {
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
