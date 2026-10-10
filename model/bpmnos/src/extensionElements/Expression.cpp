#include "Expression.h"
#include "model/bpmnos/src/Model.h"
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/Keywords.h"
#include <format>
#include <limits>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cassert>

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

  determineUses();

  for ( auto& name : compiled.getVariables() ) {
    if ( name != BPMNOS::Keyword::Undefined ) {
      auto attribute = attributeRegistry[ name ];
      if ( attribute->isObject() ) {
        throw std::runtime_error("Expression: object '" + name + "' must be indexed to a value in '" + expression +"'");
      }
      inputs.insert(attribute);
      variables.push_back(attribute);
    }
  }
  auto& collectionNames = compiled.getCollections();
  for ( size_t k = 0; k < collectionNames.size(); k++ ) {
    auto& name = collectionNames[k];
    if ( name == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal expression '" + expression +"'");
    }
    auto attribute = attributeRegistry[ name ];
    if ( attribute->isObject() ) {
      auto& schema = *attribute->schema;
      if ( schema.dimensions.empty() ) {
        throw std::runtime_error("Expression: object '" + name + "' is not an array in '" + expression +"'");
      }
      if ( !sizeOnly[k] && ( !schema.scalar.has_value() || schema.dimensions.size() != 1 ) ) {
        throw std::runtime_error("Expression: '" + name + "' is not an array of values with a single dimension in '" + expression +"'");
      }
    }
    else if ( attribute->type != BPMNOS::ValueType::COLLECTION ) {
      throw std::runtime_error("Expression: '" + name + "' is not an array in '" + expression +"'");
    }
    inputs.insert(attribute);
    collections.push_back(attribute);
  }
  auto& pathDescriptions = compiled.getPaths();
  for ( size_t path = 0; path < pathDescriptions.size(); path++ ) {
    auto attribute = attributeRegistry[ pathDescriptions[path].name ];
    bind(path, attribute);
    inputs.insert(attribute);
    paths.push_back(attribute);
  }
}

namespace {

/// Returns the text of a path, an index being written `[]`.
std::string pathText(const LIMEX::Path& path) {
  std::string text = path.name;
  for ( auto& step : path.steps ) {
    text += step.has_value() ? "." + step.value() : "[]";
  }
  return text;
}

/// The part of an object a path addresses: the layout of the base or field reached, the position of its first
/// value, and the number of its dimensions indexed.
struct Location {
  const BPMNOS::Object::Layout* layout;
  size_t offset;
  size_t dimension;
};

/// Thrown when a path reads an undefined value or is indexed by one, which makes the expression undefined.
struct Undefined {};

/// Returns the number of values one element of the given dimension of a layout spans.
size_t span(const BPMNOS::Object::Layout& layout, size_t dimension) {
  size_t result = layout.stride;
  for ( size_t d = dimension + 1; d < layout.dimensions.size(); d++ ) {
    result *= layout.dimensions[d];
  }
  return result;
}

/// Follows the steps of a path through an object, the indices counting from one.
Location locate(const BPMNOS::Object& object, const LIMEX::Path& path, const std::vector<double>& indices) {
  Location location{ object.layout.get(), 0, 0 };
  size_t next = 0;
  for ( auto& step : path.steps ) {
    if ( step.has_value() ) {
      // the fields of an element, every dimension being indexed, as the schema guarantees
      auto& fields = location.layout->fields;
      auto field = std::ranges::find_if(fields, [&step](auto& candidate) { return candidate.name == step.value(); });
      assert( field != fields.end() );
      location = Location{ &field->layout, location.offset + field->offset, 0 };
      continue;
    }
    double index = indices[next++];
    if ( std::isnan(index) ) {
      throw Undefined{};
    }
    auto& layout = *location.layout;
    if ( index < 1 || index != std::floor(index) || (size_t)index > layout.dimensions[location.dimension] ) {
      throw std::runtime_error(std::format("Expression: illegal index {} for '{}' of size {}", index, pathText(path), layout.dimensions[location.dimension]));
    }
    location.offset += ( (size_t)index - 1 ) * span(layout, location.dimension);
    location.dimension++;
  }
  return location;
}

/// Returns a view of an array of scalar values with a single dimension, an undefined value being a quiet NaN.
LIMEX::View<double> valuesOf(const BPMNOS::Object* object, size_t offset, size_t count, size_t step) {
  return LIMEX::View<double>( count, [object, offset, step](size_t k) -> double {
    auto& value = object->values[offset + k * step];
    return value.has_value() ? (double)value.value() : std::numeric_limits<double>::quiet_NaN();
  });
}

/// Returns a view of the given length whose values are never read, for the argument of `size`.
LIMEX::View<double> lengthOf(size_t count) {
  return LIMEX::View<double>( count, [](size_t) -> double { return std::numeric_limits<double>::quiet_NaN(); });
}

/// Returns the array or object an attribute holds, nullptr if it holds none.
template <typename DataType>
const BPMNOS::Object* objectOf(const AttributeRegistry& attributeRegistry, const Attribute* attribute, const BPMNOS::Status& status, const DataType& data) {
  if ( attribute->isObject() ) {
    return attributeRegistry.getObject(attribute, status, data).get();
  }
  // a collection attribute holds the index of a constant array in the object registry
  auto collection = attributeRegistry.getValue(attribute, status, data);
  return collection.has_value() ? objectRegistry[(size_t)collection.value()].get() : nullptr;
}

} // namespace

void Expression::determineUses() {
  pathUses.resize( compiled.getPaths().size() );
  sizeOnly.assign( compiled.getCollections().size(), true );
  auto& aggregatorNames = handle.getAggregatorNames();
  std::function<void(const LIMEX::Node<double>&, bool, bool)> visit = [&](const LIMEX::Node<double>& node, bool aggregated, bool sizeArgument) {
    if ( node.type == LIMEX::Type::collection ) {
      if ( !sizeArgument ) {
        sizeOnly[ std::get<size_t>(node.operands[0]) ] = false;
      }
      return;
    }
    if ( node.type == LIMEX::Type::path || node.type == LIMEX::Type::collection_path ) {
      pathUses[ std::get<size_t>(node.operands[0]) ] = Use{ node.type == LIMEX::Type::collection_path, sizeArgument, aggregated };
    }
    bool sizeChild = ( node.type == LIMEX::Type::aggregation && aggregatorNames[ std::get<size_t>(node.operands[0]) ] == "size" );
    bool aggregatedChild = aggregated || node.type == LIMEX::Type::aggregation_over;
    for ( auto& operand : node.operands ) {
      if ( std::holds_alternative< LIMEX::Node<double> >(operand) ) {
        visit( std::get< LIMEX::Node<double> >(operand), aggregatedChild, sizeChild );
      }
    }
  };
  visit(compiled.getRoot(), false, false);
}

void Expression::bind(size_t path, const Attribute* attribute) const {
  auto& description = compiled.getPaths()[path];
  auto& use = pathUses[path];
  auto text = pathText(description);
  if ( !attribute->isObject() ) {
    // a collection attribute holds a constant array, of which a path addresses an element
    if ( attribute->type != BPMNOS::ValueType::COLLECTION ) {
      throw std::runtime_error("Expression: '" + attribute->name + "' is not an array in '" + expression +"'");
    }
    if ( description.steps.size() != 1 || description.steps.front().has_value() || use.array ) {
      throw std::runtime_error("Expression: '" + text + "' must address an element of '" + attribute->name + "' in '" + expression +"'");
    }
    return;
  }
  // the steps are followed through the schema: every index takes the next dimension, and a field may only
  // follow once every dimension is indexed
  const Schema* schema = attribute->schema.get();
  size_t dimension = 0;
  for ( auto& step : description.steps ) {
    if ( !step.has_value() ) {
      if ( dimension == schema->dimensions.size() ) {
        throw std::runtime_error("Expression: '" + text + "' has more indices than '" + attribute->name + "' has dimensions in '" + expression +"'");
      }
      dimension++;
      continue;
    }
    if ( dimension < schema->dimensions.size() ) {
      throw std::runtime_error("Expression: field '" + step.value() + "' of an array in '" + text + "', whose dimensions must be indexed first, in '" + expression +"'");
    }
    auto field = std::ranges::find_if(schema->fields, [&step](auto& candidate) { return candidate.first == step.value(); });
    if ( field == schema->fields.end() ) {
      throw std::runtime_error("Expression: unknown field '" + step.value() + "' in '" + text + "' in '" + expression +"'");
    }
    schema = &field->second;
    dimension = 0;
  }
  if ( use.sizeOnly ) {
    if ( dimension == schema->dimensions.size() ) {
      throw std::runtime_error("Expression: '" + text + "' is not an array in '" + expression +"'");
    }
  }
  else if ( use.array ) {
    if ( !schema->scalar.has_value() || schema->dimensions.size() != dimension + 1 ) {
      throw std::runtime_error("Expression: '" + text + "' is not an array of values with a single dimension in '" + expression +"'");
    }
  }
  else if ( !schema->scalar.has_value() || dimension != schema->dimensions.size() ) {
    throw std::runtime_error("Expression: '" + text + "' does not address a value in '" + expression +"'");
  }
}

std::optional<double> Expression::evaluate(const std::vector<double>& variableValues, const std::vector<const BPMNOS::Object*>& collectionObjects, const std::vector<const BPMNOS::Object*>& pathObjects) const {
  assert( collectionObjects.size() == collections.size() );
  assert( pathObjects.size() == paths.size() );
  // the arrays the collections name, of which only the length is read for the argument of size
  std::vector< LIMEX::View<double> > collectionValues;
  for ( size_t k = 0; k < collectionObjects.size(); k++ ) {
    auto object = collectionObjects[k];
    auto& layout = *object->layout;
    if ( sizeOnly[k] ) {
      collectionValues.push_back( lengthOf(layout.dimensions.front()) );
    }
    else if ( object->isVector() ) {
      collectionValues.push_back( valuesOf(object, 0, layout.dimensions.front(), 1) );
    }
    else {
      throw std::runtime_error("Expression: '" + collections[k]->name + "' is not an array of values");
    }
  }
  auto& descriptions = compiled.getPaths();
  LIMEX::Resolver<double> resolver;
  resolver.value = [&](size_t path, const std::vector<double>& indices) -> double {
    try {
      auto location = locate(*pathObjects[path], descriptions[path], indices);
      auto& value = pathObjects[path]->values[location.offset];
      if ( !value.has_value() ) {
        throw Undefined{};
      }
      return (double)value.value();
    }
    catch ( const Undefined& ) {
      // an aggregation skips an undefined value, which elsewhere makes the expression undefined
      if ( pathUses[path].aggregated ) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      throw;
    }
  };
  resolver.collection = [&](size_t path, const std::vector<double>& indices) -> LIMEX::View<double> {
    auto location = locate(*pathObjects[path], descriptions[path], indices);
    auto& layout = *location.layout;
    if ( pathUses[path].sizeOnly ) {
      return lengthOf(layout.dimensions[location.dimension]);
    }
    return valuesOf(pathObjects[path], location.offset, layout.dimensions[location.dimension], span(layout, location.dimension));
  };
  try {
    return compiled.evaluate(variableValues, collectionValues, resolver);
  }
  catch ( const Undefined& ) {
    return std::nullopt;
  }
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
  
  // the arrays the collections name and the arrays or objects the paths address
  std::vector<const BPMNOS::Object*> collectionObjects;
  for ( auto attribute : collections ) {
    auto object = objectOf(attributeRegistry, attribute, status, data);
    if ( !object ) {
      // return nullopt because required collection is not given
      return std::nullopt;
    }
    collectionObjects.push_back(object);
  }
  std::vector<const BPMNOS::Object*> pathObjects;
  for ( auto attribute : paths ) {
    auto object = objectOf(attributeRegistry, attribute, status, data);
    if ( !object ) {
      // return nullopt because required collection is not given
      return std::nullopt;
    }
    pathObjects.push_back(object);
  }

  try {
    return evaluate(variableValues,collectionObjects,pathObjects);
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

