#include "Attribute.h"
#include "model/utility/src/Keywords.h"
#include "Parameter.h"
#include "Expression.h"
#include "model/utility/src/encode_collection.h"
#include "model/utility/src/encode_quoted_strings.h"

using namespace BPMNOS::Model;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry)
  : element(attribute)
  , category(category)
  , index(std::numeric_limits<size_t>::max())
  , id(attribute->id.value.value)
  , expression(getExpression(attribute->name.value.value,attributeRegistry))
  , name(getName(attribute->name.value.value))
{
//std::cerr << "Attribute: " << name << std::endl;
  attributeRegistry.add(this); 
  if ( expression ) {
    // expression requires pointer to target attribute
    const_cast<Expression*>(expression.get())->target = std::make_optional<const Attribute*>(this);
  }

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
    if ( attribute->objective.has_value() && attribute->objective->get().value.value != "none" ) {
      throw std::runtime_error("Attribute: required objective weight missing for attribute '" + id + "'");
    }
    weight = 0;
  }
  
  isImmutable = (id != Keyword::Timestamp);
}

std::unique_ptr<const Expression> Attribute::getExpression(std::string& input, AttributeRegistry& attributeRegistry) {
  if ( !input.contains(":=") ) {
    return nullptr;
  }

  auto expression = std::make_unique<const Expression>(encodeCollection(encodeQuotedStrings(input)),attributeRegistry,true);
  auto& root = expression->compiled.getRoot(); 
  assert( root.operands.size() == 1 );
  assert( root.type == LIMEX::Type::group );
  auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);
  if ( node.type != LIMEX::Type::assign ) {
    throw std::runtime_error("Attribute: illegal initialization '" + input + "' for attribute '" + id + "'"); 
  }
  return expression;
}

std::string Attribute::getName(std::string& input) {
  if ( expression ) {
    assert( expression->compiled.getTarget().has_value() );
    return expression->compiled.getTarget().value();
  }
  return input;
}


