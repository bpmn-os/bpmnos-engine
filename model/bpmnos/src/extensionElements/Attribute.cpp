#include "Attribute.h"
#include "ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include "Parameter.h"
#include "Expression.h"
#include "model/utility/src/InputEncoder.h"

using namespace BPMNOS::Model;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry)
  : element(attribute)
  , category(category)
  , index(std::numeric_limits<size_t>::max())
  , id(attribute->id.value.value)
  , schema(getSchema(attribute->type.value.value))
  , expression(getExpression(attribute->name.value.value,attributeRegistry))
  , name(getName(attribute->name.value.value))
{
//std::cerr << "Attribute: " << name << std::endl;
  // the type of a scalar attribute, the type having been parsed and checked when the schema was determined
  type = schema ? schema->scalar.value_or(ValueType::DECIMAL) : Schema::parse(attribute->type.value.value).scalar.value();
  if ( schema ) {
    if ( id == Keyword::Instance || id == Keyword::Timestamp ) {
      throw std::runtime_error("Attribute: '" + id + "' must not be an object");
    }
    if ( expression ) {
      throw std::runtime_error("Attribute: object '" + id + "' must not have an initial expression");
    }
  }
  // the registry numbers objects separately from scalar attributes, so it must know which this is
  attributeRegistry.add(this);
  if ( id == Keyword::Timestamp && index != ExtensionElements::Index::Timestamp ) {
    throw std::runtime_error("Attribute: timestamp must be first status attribute");
  }
  if ( expression ) {
    // expression requires pointer to target attribute
    const_cast<Expression*>(expression.get())->target = std::make_optional<const Attribute*>(this);
  }

  if ( attribute->weight.has_value() && ( schema || ( type != ValueType::BOOLEAN && type != ValueType::INTEGER && type != ValueType::DECIMAL ) ) ) {
    throw std::runtime_error("Attribute: objective of attribute '" + id + "' requires type boolean, integer, or decimal");
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

  auto expression = std::make_unique<const Expression>(InputEncoder(input),attributeRegistry,true);
  auto& root = expression->compiled.getRoot(); 
  assert( root.operands.size() == 1 );
  assert( root.type == LIMEX::Type::group );
  auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);
  if ( node.type != LIMEX::Type::assign ) {
    throw std::runtime_error("Attribute: illegal initialization '" + input + "' for attribute '" + id + "'"); 
  }
  return expression;
}

std::unique_ptr<const Schema> Attribute::getSchema(const std::string& input) {
  Schema parsed;
  try {
    parsed = Schema::parse(input);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("Attribute: illegal type of attribute '" + id + "'.\n" + error.what());
  }
  if ( parsed.isScalar() ) {
    return nullptr;
  }
  return std::make_unique<const Schema>(std::move(parsed));
}

std::string Attribute::getName(std::string& input) {
  if ( expression ) {
    assert( expression->compiled.getTarget().has_value() );
    return expression->compiled.getTarget().value();
  }
  return input;
}


