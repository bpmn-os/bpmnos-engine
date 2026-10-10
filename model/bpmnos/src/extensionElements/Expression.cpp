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
#include <regex>

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
  resize = ( withoutResize(expression) != expression );
  auto& assignment = std::get< LIMEX::Node<double> >(compiled.getRoot().operands[0]);
  compound = ( (int)assignment.type > (int)LIMEX::Type::assign );
  if ( auto name = compiled.getTarget(); name.has_value() ) {
    if ( name.value() == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal assignment '" + expression +"'");
    }
    if ( attributeRegistry.inputs.contains(name.value()) ) {
      throw std::runtime_error("Expression: input '" + name.value() + "' is read only and cannot be assigned in '" + expression + "'");
    }
    if ( !newTarget ) {
      target = attributeRegistry[ name.value() ];
      objectTarget = target.value()->isObject();
    }
  }
  if ( resize && !objectTarget ) {
    throw std::runtime_error("Expression: resize requires an array in '" + expression + "'");
  }
  if ( compiled.getTargetPath().has_value() && !objectTarget ) {
    throw std::runtime_error("Expression: assignment to an element of '" + compiled.getTarget().value() + "', which is not an object, in '" + expression + "'");
  }
  if ( objectTarget ) {
    if ( type != Type::ASSIGN ) {
      throw std::runtime_error("Expression: object '" + target.value()->name + "' cannot be undefined in '" + expression + "'");
    }
    if ( compound && !compiled.getTargetPath().has_value() ) {
      throw std::runtime_error("Expression: object '" + target.value()->name + "' must be assigned by ':=' in '" + expression + "'");
    }
    analyseTarget(encoder.objects());
  }

  determineUses();

  for ( auto& name : compiled.getVariables() ) {
    if ( auto input = attributeRegistry.inputs.find(name); input != attributeRegistry.inputs.end() ) {
      // an input is read as a value only through a path, unless it is the object assigned
      if ( input->second != sourceInput ) {
        throw std::runtime_error("Expression: input '" + name + "' must be indexed to a value in '" + expression +"'");
      }
      variables.push_back(nullptr);
      continue;
    }
    if ( name != BPMNOS::Keyword::Undefined ) {
      auto attribute = attributeRegistry[ name ];
      if ( attribute->isObject() && attribute != sourceObject ) {
        throw std::runtime_error("Expression: object '" + name + "' must be indexed to a value in '" + expression +"'");
      }
      inputs.insert(attribute);
      variables.push_back(attribute);
    }
  }
  auto& pathDescriptions = compiled.getPaths();
  for ( size_t path = 0; path < pathDescriptions.size(); path++ ) {
    // the target written and the source of an object assigned have been bound by the analysis of the target
    bool written = ( objectTarget && !compound && compiled.getTargetPath() == path );
    if ( !written && sourcePath != path ) {
      bind(path);
    }
    if ( auto input = attributeRegistry.inputs.find(pathDescriptions[path].name); input != attributeRegistry.inputs.end() ) {
      // an input never changes, so that no expression depends on it
      paths.push_back(nullptr);
      pathInputs.push_back(input->second);
      continue;
    }
    auto attribute = attributeRegistry[ pathDescriptions[path].name ];
    inputs.insert(attribute);
    paths.push_back(attribute);
    pathInputs.push_back(nullptr);
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

/// Returns the layout of the part of an object a location addresses, which holds the dimensions not indexed.
BPMNOS::Object::Layout elementLayout(const Location& location) {
  auto& layout = *location.layout;
  BPMNOS::Object::Layout element;
  element.scalar = layout.scalar;
  element.fields = layout.fields;
  element.stride = layout.stride;
  element.dimensions.assign(layout.dimensions.begin() + (long)location.dimension, layout.dimensions.end());
  if ( !layout.fixed.empty() ) {
    element.fixed.assign(layout.fixed.begin() + (long)location.dimension, layout.fixed.end());
  }
  return element;
}

/// Returns a copy of the part of an object a location addresses, whose values are contiguous.
std::shared_ptr<const BPMNOS::Object> part(const BPMNOS::Object& object, const Location& location) {
  auto result = std::make_shared<BPMNOS::Object>();
  result->layout = std::make_shared<const BPMNOS::Object::Layout>( elementLayout(location) );
  auto begin = object.values.begin() + (long)location.offset;
  result->values.assign(begin, begin + (long)result->layout->size());
  return result;
}

/// Returns the array or object an object attribute holds.
template <typename DataType>
const BPMNOS::Object* objectOf(const AttributeRegistry& attributeRegistry, const Attribute* attribute, const BPMNOS::Status& status, const DataType& data) {
  return attributeRegistry.getObject(attribute, status, data).get();
}

} // namespace

void Expression::determineUses() {
  pathUses.resize( compiled.getPaths().size() );
  auto& aggregatorNames = handle.getAggregatorNames();
  std::function<void(const LIMEX::Node<double>&, bool, bool)> visit = [&](const LIMEX::Node<double>& node, bool aggregated, bool sizeArgument) {
    if ( node.type == LIMEX::Type::path || node.type == LIMEX::Type::collection_path ) {
      pathUses[ std::get<size_t>(node.operands[0]) ] = Use{ node.type == LIMEX::Type::collection_path, sizeArgument, aggregated };
    }
    if ( node.type == LIMEX::Type::function_call && &node != sourceCall ) {
      // a lookup returning an array may only be the entire value of an assignment to an array
      auto& name = handle.getFunctionNames()[ std::get<size_t>(node.operands[0]) ];
      if ( attributeRegistry.arrayLookups.contains(name) ) {
        throw std::runtime_error("Expression: lookup '" + name + "' returns an array, which may only be assigned to an array, in '" + expression + "'");
      }
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

const Schema* Expression::schemaOf(const std::string& name) const {
  if ( auto input = attributeRegistry.inputs.find(name); input != attributeRegistry.inputs.end() ) {
    return &input->second->schema;
  }
  return attributeRegistry[name]->schema.get();
}

Expression::Step Expression::walk(size_t path) const {
  auto& description = compiled.getPaths()[path];
  auto text = pathText(description);
  auto schema = schemaOf(description.name);
  if ( !schema ) {
    throw std::runtime_error("Expression: '" + text + "' does not address an object in '" + expression +"'");
  }
  // the steps are followed through the schema: every index takes the next dimension, and a field may only
  // follow once every dimension is indexed
  Step result{ schema, 0, {} };
  for ( auto& step : description.steps ) {
    if ( !step.has_value() ) {
      if ( result.dimension == result.schema->dimensions.size() ) {
        throw std::runtime_error("Expression: '" + text + "' has more indices than '" + description.name + "' has dimensions in '" + expression +"'");
      }
      result.dimension++;
      continue;
    }
    if ( result.dimension < result.schema->dimensions.size() ) {
      throw std::runtime_error("Expression: field '" + step.value() + "' of an array in '" + text + "', whose dimensions must be indexed first, in '" + expression +"'");
    }
    auto field = std::ranges::find_if(result.schema->fields, [&step](auto& candidate) { return candidate.first == step.value(); });
    if ( field == result.schema->fields.end() ) {
      throw std::runtime_error("Expression: unknown field '" + step.value() + "' in '" + text + "' in '" + expression +"'");
    }
    result.schema = &field->second;
    result.dimension = 0;
    result.fields.push_back(step.value());
  }
  return result;
}

void Expression::bind(size_t path) const {
  auto& use = pathUses[path];
  auto& description = compiled.getPaths()[path];
  auto text = pathText(description);
  if ( description.steps.empty() && !schemaOf(description.name) ) {
    throw std::runtime_error("Expression: '" + description.name + "' is not an array in '" + expression +"'");
  }
  auto [schema, dimension, _] = walk(path);
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

void Expression::analyseTarget(const std::vector<size_t>& literals) {
  auto attribute = target.value();
  auto targetPath = compiled.getTargetPath();
  if ( resize ) {
    // the length of the dimension the target addresses is assigned a scalar value
    if ( targetPath.has_value() ) {
      auto step = walk(targetPath.value());
      if ( step.dimension == step.schema->dimensions.size() ) {
        throw std::runtime_error("Expression: '" + pathText(compiled.getPaths()[targetPath.value()]) + "' is not an array in '" + expression + "'");
      }
      resizeFields = step.fields;
      resizeDimension = step.dimension;
    }
    else if ( attribute->schema->dimensions.empty() ) {
      throw std::runtime_error("Expression: object '" + attribute->name + "' is not an array in '" + expression + "'");
    }
    return;
  }
  if ( compound ) {
    // the target is read and written as a value, and bound as such
    return;
  }
  if ( targetPath.has_value() ) {
    auto step = walk(targetPath.value());
    objectValue = !( step.schema->scalar.has_value() && step.dimension == step.schema->dimensions.size() );
    targetFields = step.fields;
    targetDimension = step.dimension;
  }
  else {
    objectValue = true;
  }
  if ( !objectValue ) {
    return;
  }
  // an array or object is assigned a literal, the object of an attribute, or the part of an object a path
  // addresses
  auto& assignment = std::get< LIMEX::Node<double> >(compiled.getRoot().operands[0]);
  auto& value = std::get< LIMEX::Node<double> >(assignment.operands[0]);
  auto illegal = [&]() {
    return std::runtime_error("Expression: '" + attribute->name + "' must be assigned a literal, an object, or a path to an array or object in '" + expression + "'");
  };
  if ( value.type == LIMEX::Type::literal ) {
    double number = std::get<double>(value.operands[0]);
    if ( !std::ranges::contains(literals, (size_t)number) || number != std::floor(number) ) {
      throw illegal();
    }
    sourceLiteral = (size_t)number;
  }
  else if ( value.type == LIMEX::Type::variable ) {
    auto& name = compiled.getVariables()[ std::get<size_t>(value.operands[0]) ];
    if ( auto input = attributeRegistry.inputs.find(name); input != attributeRegistry.inputs.end() ) {
      // the object an input holds
      sourceInput = input->second;
      return;
    }
    auto source = attributeRegistry[ name ];
    if ( !source->isObject() ) {
      throw illegal();
    }
    sourceObject = source;
  }
  else if ( value.type == LIMEX::Type::function_call && attributeRegistry.arrayLookups.contains( handle.getFunctionNames()[ std::get<size_t>(value.operands[0]) ] ) ) {
    // a lookup returning an array, which arrives as the index of a constant object
    sourceCall = &value;
  }
  else if ( value.type == LIMEX::Type::path ) {
    size_t path = std::get<size_t>(value.operands[0]);
    auto step = walk(path);
    if ( step.schema->scalar.has_value() && step.dimension == step.schema->dimensions.size() ) {
      throw illegal();
    }
    sourcePath = path;
  }
  else {
    throw illegal();
  }
}

std::string Expression::withoutResize(const std::string& text) {
  // 'resize(path) := n' is read as the assignment 'path := n' of the length of the dimension the path addresses
  static const std::regex form(R"(^\s*resize\s*\((.*)\)\s*:=(.*)$)");
  std::smatch match;
  if ( std::regex_match(text, match, form) ) {
    return match[1].str() + " :=" + match[2].str();
  }
  return text;
}

LIMEX::Resolver<double> Expression::resolverOf(const std::vector<const BPMNOS::Object*>& pathObjects) const {
  auto& descriptions = compiled.getPaths();
  LIMEX::Resolver<double> resolver;
  resolver.value = [this, &descriptions, &pathObjects](size_t path, const std::vector<double>& indices) -> double {
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
  resolver.collection = [this, &descriptions, &pathObjects](size_t path, const std::vector<double>& indices) -> LIMEX::View<double> {
    auto location = locate(*pathObjects[path], descriptions[path], indices);
    auto& layout = *location.layout;
    if ( pathUses[path].sizeOnly ) {
      return lengthOf(layout.dimensions[location.dimension]);
    }
    return valuesOf(pathObjects[path], location.offset, layout.dimensions[location.dimension], span(layout, location.dimension));
  };
  return resolver;
}

std::optional<double> Expression::evaluate(const std::vector<double>& variableValues, const std::vector<const BPMNOS::Object*>& pathObjects) const {
  assert( pathObjects.size() == paths.size() );
  auto resolver = resolverOf(pathObjects);
  try {
    auto value = compiled.evaluate(variableValues, resolver);
    if ( std::isnan(value) ) {
      return std::nullopt;
    }
    return value;
  }
  catch ( const Undefined& ) {
    return std::nullopt;
  }
}

LIMEX::Expression<double> Expression::getExpression(const std::string& input) const {
  try {
    // the text has been scanned by the caller, so that every literal in it is a number already
    return LIMEX::Expression<double>(withoutResize(input), handle);
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

  std::vector<double> variableValues;
  std::vector<const BPMNOS::Object*> pathObjects;
  if ( !gather(status, data, variableValues, pathObjects, false) ) {
    // return nullopt because a required value is not given
    return std::nullopt;
  }

  try {
    return evaluate(variableValues,pathObjects);
  }
  catch (const std::runtime_error& e) {
    throw std::runtime_error(std::format("Expression: failed to evaluate '{}' with {}\n{}", expression, arguments(status, data), e.what()));
  }
}

template <typename DataType>
std::string Expression::arguments(const BPMNOS::Status& status, const DataType& data) const {
  std::string result;
  for ( auto attribute : variables ) {
    if ( !attribute || attribute->isObject() ) {
      continue;
    }
    result += ( result.empty() ? "" : ", " ) + attribute->name + " = ";
    auto value = attributeRegistry.getValue(attribute,status,data);
    result += value.has_value() ? BPMNOS::to_string(value.value(),attribute->type) : "undefined";
  }
  return result;
}

template <typename DataType>
bool Expression::gather(const BPMNOS::Status& status, const DataType& data, std::vector<double>& variableValues, std::vector<const BPMNOS::Object*>& pathObjects, bool undefinedAsNaN) const {
  for ( auto attribute : variables ) {
    if ( !attribute || attribute->isObject() ) {
      // the object assigned, which LIMEX reads as a variable whose value is not used
      variableValues.push_back(0);
      continue;
    }
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( !value.has_value() && !undefinedAsNaN ) {
      return false;
    }
    variableValues.push_back( value.has_value() ? (double)value.value() : std::numeric_limits<double>::quiet_NaN() );
  }
  // the arrays and objects the paths address, a name used as an array being a path without steps
  for ( size_t path = 0; path < paths.size(); path++ ) {
    pathObjects.push_back( paths[path] ? objectOf(attributeRegistry, paths[path], status, data) : pathInputs[path]->object.get() );
    if ( !pathObjects.back() ) {
      return false;
    }
  }
  return true;
}

template <typename DataType>
void Expression::write(BPMNOS::Status& status, DataType& data) const {
  assert( objectTarget );
  std::vector<double> variableValues;
  std::vector<const BPMNOS::Object*> pathObjects;
  if ( !gather(status, data, variableValues, pathObjects, true) ) {
    throw std::runtime_error("Expression: '" + expression + "' reads an array without a value");
  }
  auto& slot = attributeRegistry.getObjectSlot(target.value(), status, data);
  auto targetPath = compiled.getTargetPath();
  try {
    auto resolver = resolverOf(pathObjects);
    // the indices of the target, all of which must be defined
    std::vector<double> indices;
    try {
      indices = compiled.evaluateTargetIndices(variableValues, resolver);
    }
    catch ( const Undefined& ) {
      throw std::runtime_error("Expression: undefined index");
    }
    auto locateTarget = [&]() {
      try {
        return locate(*slot, compiled.getPaths()[targetPath.value()], indices);
      }
      catch ( const Undefined& ) {
        throw std::runtime_error("Expression: undefined index");
      }
    };

    if ( resize ) {
      auto length = evaluate(variableValues, pathObjects);
      if ( !length.has_value() || length.value() < 0 || length.value() != std::floor(length.value()) ) {
        throw std::runtime_error("Expression: resize requires a non-negative integer");
      }
      if ( targetPath.has_value() ) {
        // the indices of the path are checked, the dimension changing in every element alike
        locateTarget();
      }
      slot = BPMNOS::resized(*slot, resizeFields, resizeDimension, (size_t)length.value());
      return;
    }

    if ( !objectValue ) {
      // a value is written to an element, which never changes a length
      auto value = evaluate(variableValues, pathObjects);
      auto location = locateTarget();
      modifiable(slot).values[location.offset] = BPMNOS::to_value(value);
      return;
    }

    // the array or object assigned
    std::shared_ptr<const BPMNOS::Object> source;
    if ( sourceLiteral.has_value() ) {
      source = objectRegistry[sourceLiteral.value()];
    }
    else if ( sourceCall ) {
      // the value of the assignment is the index of the constant object the lookup returns
      double index = 0;
      try {
        index = compiled.evaluate(variableValues, resolver);
      }
      catch ( const Undefined& ) {
        throw std::runtime_error("Expression: undefined argument");
      }
      source = objectRegistry[(size_t)index];
    }
    else if ( sourceObject ) {
      source = attributeRegistry.getObject(sourceObject, status, data);
    }
    else if ( sourceInput ) {
      source = sourceInput->object;
    }
    else {
      // the indices of the source path are those LIMEX evaluates for the value of the assignment
      std::vector<double> sourceIndices;
      auto capturing = resolver;
      capturing.value = [&, inner = resolver.value](size_t path, const std::vector<double>& pathIndices) -> double {
        if ( path == sourcePath.value() ) {
          sourceIndices = pathIndices;
          return 0;
        }
        return inner(path, pathIndices);
      };
      try {
        compiled.evaluate(variableValues, capturing);
        source = part(*pathObjects[sourcePath.value()], locate(*pathObjects[sourcePath.value()], compiled.getPaths()[sourcePath.value()], sourceIndices));
      }
      catch ( const Undefined& ) {
        throw std::runtime_error("Expression: undefined index");
      }
    }

    if ( !targetPath.has_value() ) {
      // the whole object keeps the lengths of its fixed dimensions and takes those of its open ones
      attributeRegistry.setObject(target.value(), status, data, source);
      return;
    }

    // an element keeps its lengths, except those not yet determined, which the value determines for every
    // element of the array
    auto location = locateTarget();
    auto element = elementLayout(location);
    auto value = Schema::of(element, true).conform(source);
    std::function<void(const BPMNOS::Object::Layout&, const BPMNOS::Object::Layout&, std::vector<std::string>, size_t)> determine =
      [&](const BPMNOS::Object::Layout& current, const BPMNOS::Object::Layout& given, std::vector<std::string> fields, size_t first) {
        for ( size_t d = 0; d < current.dimensions.size(); d++ ) {
          if ( current.dimensions[d] == 0 && given.dimensions[d] > 0 ) {
            slot = BPMNOS::resized(*slot, fields, first + d, given.dimensions[d]);
          }
        }
        for ( size_t k = 0; k < current.fields.size(); k++ ) {
          auto nested = fields;
          nested.push_back(current.fields[k].name);
          determine(current.fields[k].layout, given.fields[k].layout, nested, 0);
        }
      };
    determine(element, *value->layout, targetFields, targetDimension);
    location = locateTarget();
    auto& values = modifiable(slot).values;
    assert( location.offset + value->values.size() <= values.size() );
    std::ranges::copy(value->values, values.begin() + (long)location.offset);
  }
  catch ( const std::runtime_error& error ) {
    throw std::runtime_error(std::format("Expression: failed to evaluate '{}' with {}\n{}", expression, arguments(status, data), error.what()));
  }
}

template std::optional<double> Expression::execute<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template std::optional<double> Expression::execute<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
template void Expression::write<BPMNOS::Data>(BPMNOS::Status& status, BPMNOS::Data& data) const;
template void Expression::write<BPMNOS::SharedData>(BPMNOS::Status& status, BPMNOS::SharedData& data) const;

