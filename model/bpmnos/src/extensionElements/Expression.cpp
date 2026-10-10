#include "Expression.h"
#include "model/bpmnos/src/Model.h"
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/Keywords.h"
#include <format>
#include <limits>
#include <functional>

using namespace BPMNOS::Model;

Expression::Expression(const InputEncoder& encoder, const AttributeRegistry& attributeRegistry, bool newTarget)
  : Expression(attributeRegistry.limexHandle, encoder, attributeRegistry, newTarget)
{
}

Expression::Expression(const LIMEX::Handle<double>& handle, const InputEncoder& encoder,
                       const AttributeRegistry& attributeRegistry, bool newTarget)
  : attributeRegistry(attributeRegistry)
  , handle(handle)
  , expression(encoder.text())
  , compiled(getExpression(expression))
  , type(getType())
{
  if ( compiled.getTargetPath().has_value() ) {
    throw std::runtime_error("Expression: assignment to an element of '" + compiled.getTarget().value() + "' is not supported in '" + expression + "'");
  }
  if ( auto name = compiled.getTarget(); name.has_value() ) {
    if ( name.value() == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal assignment '" + expression +"'");
    }
    if ( !newTarget ) {
      target = attributeRegistry[ name.value() ];
      if ( target.value()->isObject() ) {
        throw std::runtime_error("Expression: object '" + name.value() + "' cannot be assigned in '" + expression +"'");
      }
    }
  }

  for ( auto& name : compiled.getVariables() ) {
    if ( name != BPMNOS::Keyword::Undefined ) {
      auto attribute = attributeRegistry[ name ];
      if ( attribute->isObject() ) {
        throw std::runtime_error("Expression: object '" + name + "' cannot be used in '" + expression +"'");
      }
      inputs.insert(attribute);
      variables.push_back(attribute);
    }
  }
  for ( auto& name : compiled.getCollections() ) {
    if ( name == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal expression '" + expression +"'");
    }
    auto attribute = attributeRegistry[ name ];
    if ( attribute->isObject() ) {
      throw std::runtime_error("Expression: object '" + name + "' cannot be used in '" + expression +"'");
    }
    inputs.insert(attribute);
    collections.push_back(attribute);
  }
  for ( auto& path : compiled.getPaths() ) {
    if ( path.steps.size() != 1 || path.steps.front().has_value() ) {
      throw std::runtime_error("Expression: '" + path.name + "' must be indexed exactly once in '" + expression +"'");
    }
    auto attribute = attributeRegistry[ path.name ];
    if ( attribute->isObject() ) {
      throw std::runtime_error("Expression: object '" + path.name + "' cannot be used in '" + expression +"'");
    }
    if ( attribute->type != BPMNOS::ValueType::COLLECTION ) {
      throw std::runtime_error("Expression: '" + path.name + "' is not a collection in '" + expression +"'");
    }
    inputs.insert(attribute);
    paths.push_back(attribute);
  }
  // a path may only address an element, not an array
  std::function<void(const LIMEX::Node<double>&)> rejectArrays = [&](const LIMEX::Node<double>& node) {
    if ( node.type == LIMEX::Type::collection_path ) {
      throw std::runtime_error("Expression: an element of a collection cannot be used as an array in '" + expression +"'");
    }
    for ( auto& operand : node.operands ) {
      if ( std::holds_alternative< LIMEX::Node<double> >(operand) ) {
        rejectArrays( std::get< LIMEX::Node<double> >(operand) );
      }
    }
  };
  rejectArrays(compiled.getRoot());
}

double Expression::evaluate(const std::vector<double>& variableValues, const std::vector< LIMEX::View<double> >& collectionValues, const std::vector<BPMNOS::number>& pathCollections) const {
  assert( pathCollections.size() == paths.size() );
  LIMEX::Resolver<double> resolver;
  resolver.value = [this, &pathCollections](size_t path, const std::vector<double>& indices) -> double {
    // a path addresses the element of a constant array at its index, counting from one
    const auto& collection = *objectRegistry[(size_t)pathCollections[path]];
    if ( !collection.isVector() ) {
      throw std::runtime_error("Expression: '" + paths[path]->name + "' is not an array of values");
    }
    if ( indices.front() < 1 || (size_t)indices.front() - 1 >= collection.values.size() ) {
      throw std::runtime_error(std::format("Expression: illegal index {} for '{}'", indices.front(), paths[path]->name));
    }
    auto& value = collection.values[(size_t)indices.front() - 1];
    return value.has_value() ? (double)value.value() : std::numeric_limits<double>::quiet_NaN();
  };
  return compiled.evaluate(variableValues, collectionValues, resolver);
}

LIMEX::View<double> Expression::view(BPMNOS::number collection) {
  // a registered object never moves and never changes, so the view may refer to its values
  const BPMNOS::Object* object = objectRegistry[(size_t)collection].get();
  if ( !object->isVector() ) {
    throw std::runtime_error("Expression: '" + BPMNOS::to_string(*object) + "' is not an array of values");
  }
  return LIMEX::View<double>( object->values.size(), [object](size_t k) -> double {
    auto& value = object->values[k];
    return value.has_value() ? (double)value.value() : std::numeric_limits<double>::quiet_NaN();
  });
}

LIMEX::Expression<double> Expression::getExpression(const std::string& input) const {
  try {
    // the text has been scanned by the caller, so that every literal in it is a number already
    return LIMEX::Expression<double>(input, handle);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("Expression: illegal expression '" + input + "'.\n" + error.what());
  }
}

Expression::Type Expression::getType() const {
  auto variableNames = compiled.getVariables();
  assert( compiled.getRoot().operands.size() == 1 );
  assert( compiled.getRoot().type == LIMEX::Type::group );

  auto& root = compiled.getRoot(); 
  assert( !root.operands.empty() );
  auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);

  // check if any of the variables is named "undefined"
  if ( std::find( variableNames.begin(), variableNames.end(), BPMNOS::Keyword::Undefined ) != variableNames.end() ) {
    // only lhs == undefined, lhs != undefined, and lhs := undefined are allowed
    
    if ( node.type == LIMEX::Type::assign ) {
      assert( node.operands.size() == 1 );
      if ( 
        !std::holds_alternative< LIMEX::Node<double> >(node.operands[0]) || 
        std::get< LIMEX::Node<double> >(node.operands[0]).type != LIMEX::Type::variable
      ) {
        throw std::runtime_error("Expression: illegal assignment '" + expression +"'");
      }
      return Type::UNASSIGN;
    }

    if ( node.type != LIMEX::Type::equal_to && node.type != LIMEX::Type::not_equal_to ) {
      throw std::runtime_error("Expression: illegal expression '" + expression +"'");
    }
    
    assert( node.operands.size() == 2 );
    assert( std::holds_alternative< LIMEX::Node<double> >(node.operands[0]) );
    assert( std::holds_alternative< LIMEX::Node<double> >(node.operands[1]) );
    auto& lhs = std::get< LIMEX::Node<double> >(node.operands[0]);
    auto& rhs = std::get< LIMEX::Node<double> >(node.operands[1]);
    assert( !lhs.operands.empty() );
    assert( !rhs.operands.empty() );

    if (
      lhs.type != LIMEX::Type::variable ||
      variableNames.front() == BPMNOS::Keyword::Undefined ||
      rhs.type != LIMEX::Type::variable ||
      variableNames.back() != BPMNOS::Keyword::Undefined
    ) {
      throw std::runtime_error("Expression: illegal comparison '" + expression +"'");
    }

    return node.type == LIMEX::Type::equal_to ? Type::IS_NULL : Type::IS_NOT_NULL;
  }
  // all variables must be defined
  return ( (int)node.type >= (int)LIMEX::Type::assign ) ? Type::ASSIGN : Type::OTHER;
}

const Attribute* Expression::isAttribute() const {
  assert( compiled.getRoot().operands.size() == 1 );
  assert( compiled.getRoot().type == LIMEX::Type::group );
  auto& root = compiled.getRoot(); 
  auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);
  if ( node.type == LIMEX::Type::variable ) {
    assert(inputs.size());
    return *inputs.begin();
  }
  return nullptr;
}

template <typename DataType>
std::optional<double> Expression::execute(const BPMNOS::Status& status, const DataType& data) const {
  if ( type == Type::UNASSIGN ) {
    return std::nullopt;
  }
  if ( type == Type::IS_NULL ) {
    assert(variables.size() == 1);
    auto value = attributeRegistry.getValue(variables[0],status,data);
    return (double)!value.has_value();
  }
  if ( type == Type::IS_NOT_NULL ) {
    assert(variables.size() == 1);
    auto value = attributeRegistry.getValue(variables[0],status,data);
    return (double)value.has_value();
  }

  // collect variable values
  std::vector< double > variableValues;
  for ( auto attribute : variables ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( !value.has_value() ) {
      // return nullopt because required attribute value is not given
      return std::nullopt;
    }
    variableValues.push_back( (double)value.value() );
  }
  
  // collect a view of each collection, the constant objects in the registry never moving
  std::vector< LIMEX::View<double> > collectionValues;
  for ( auto attribute : collections ) {
    auto collection = attributeRegistry.getValue(attribute,status,data);
    if ( !collection.has_value() ) {
      // return nullopt because required collection is not given
      return std::nullopt;
    }
    collectionValues.push_back( view(collection.value()) );
  }

  // the constant arrays the paths index
  std::vector<BPMNOS::number> pathCollections;
  for ( auto attribute : paths ) {
    auto collection = attributeRegistry.getValue(attribute,status,data);
    if ( !collection.has_value() ) {
      // return nullopt because required collection is not given
      return std::nullopt;
    }
    pathCollections.push_back( collection.value() );
  }

  try {
    return evaluate(variableValues,collectionValues,pathCollections);
  }
  catch (const std::runtime_error& e) {
    std::string arguments;
    for ( auto attribute : variables ) {
      if (attribute != variables.front()) arguments += ", ";
      arguments += attribute->name + " = ";
      auto value = attributeRegistry.getValue(attribute,status,data);
      assert( value.has_value() );
      arguments += BPMNOS::to_string(value.value(),attribute->type);
    }
    throw std::runtime_error(std::format("Expression: failed to evaluate '{}' with {}\n{}", expression, arguments, e.what()));
  }
}

template std::optional<double> Expression::execute<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template std::optional<double> Expression::execute<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

