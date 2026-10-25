#include "Attribute.h"
#include "ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include "Parameter.h"
#include "Expression.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/string_utility.h"
#include <algorithm>
#include <cctype>
#include <limits>

using namespace BPMNOS::Model;

Attribute::Attribute(XML::bpmnos::tAttribute* attribute, Attribute::Category category, AttributeRegistry& attributeRegistry)
  : element(attribute)
  , category(category)
  , index(std::numeric_limits<size_t>::max())
  , id(attribute->id.value.value)
  , type(getType(attribute->type.value.value))
  , initialization(getInitialization(attribute->name.value.value,attributeRegistry))
  , name(getName(attribute->name.value.value))
{
//std::cerr << "Attribute: " << name << std::endl;
  if ( isObject() ) {
    if ( id == Keyword::Instance || id == Keyword::Timestamp ) {
      throw std::runtime_error("Attribute: '" + id + "' must not be an object");
    }
  }
  // the registry numbers objects separately from scalar attributes, so it must know which this is
  attributeRegistry.add(this);
  if ( isObject() && std::holds_alternative<std::monostate>(initialization) && attribute->name.value.value.contains(":=") ) {
    // the assignment initialising an object other than by a literal finds the object by its name, and is therefore
    // compiled once the attribute is registered
    std::unique_ptr<const Expression> expression;
    try {
      expression = std::make_unique<const Expression>(InputEncoder(attribute->name.value.value), attributeRegistry);
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("Attribute: illegal initial value of object '" + id + "'.\n" + error.what());
    }
    if ( !expression->writesToObject() ) {
      throw std::runtime_error("Attribute: illegal initial value of object '" + id + "'");
    }
    initialization = std::move(expression);
  }
  if ( id == Keyword::Timestamp && index != ExtensionElements::Index::Timestamp ) {
    throw std::runtime_error("Attribute: timestamp must be first status attribute");
  }
  if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&initialization) ) {
    // expression requires pointer to target attribute
    const_cast<Expression*>(expression->get())->target = std::make_optional<const Attribute*>(this);
  }

  if ( attribute->weight.has_value() && ( isObject() || std::get<ValueType>(type) == ValueType::STRING ) ) {
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

Attribute::Attribute(std::string& name, const Schema& schema, AttributeRegistry& attributeRegistry)
  : element(nullptr)
  , category(Category::STATUS)
  , index(std::numeric_limits<size_t>::max())
  , id(name)
  , type(schema)
  , name(name)
  , weight(0)
  , isImmutable(false)
{
  // the operators refer to the content object by the name, which is therefore a name of an expression
  bool isName = !name.empty() && ( std::isalpha((unsigned char)name.front()) || name.front() == '_' ) &&
    std::ranges::all_of(name, [](char character) { return std::isalnum((unsigned char)character) || character == '_'; });
  if ( !isName ) {
    throw std::runtime_error("Attribute: '" + name + "' is no valid name of a message or signal");
  }
  attributeRegistry.add(this);
}

std::variant<std::monostate, size_t, std::unique_ptr<const Expression>> Attribute::getInitialization(std::string& input, AttributeRegistry& attributeRegistry) {
  if ( !input.contains(":=") ) {
    return std::monostate{};
  }

  if ( isObject() ) {
    // an object initialised with a literal, which is registered as a constant object and checked against the
    // type, is shared by every instance and needs no expression; an object initialised otherwise is written by
    // an expression compiled once the attribute is registered, when its scope is created
    auto value = BPMNOS::trim_copy( input.substr( input.find(":=") + 2 ) );
    InputEncoder encoder(value);
    if ( !encoder.getObjectIndex().has_value() ) {
      return std::monostate{};
    }
    size_t literal = encoder.getObjectIndex().value();
    try {
      // the conversion rejects a literal that does not fit the type
      if ( !type.supports(objectRegistry.schema(literal)) ) {
        BPMNOS::Object(type, *objectRegistry[literal], objectRegistry.schema(literal));
      }
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("Attribute: illegal initial value of object '" + id + "'.\n" + error.what());
    }
    return literal;
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

BPMNOS::Schema Attribute::getType(const std::string& input) {
  try {
    return Schema::parse(input);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("Attribute: illegal type of attribute '" + id + "'.\n" + error.what());
  }
}

std::string Attribute::getName(std::string& input) {
  if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&initialization) ) {
    assert( (*expression)->compiled.getTarget().has_value() );
    return (*expression)->compiled.getTarget().value();
  }
  if ( isObject() && input.contains(":=") ) {
    return BPMNOS::trim_copy( input.substr( 0, input.find(":=") ) );
  }
  return input;
}


