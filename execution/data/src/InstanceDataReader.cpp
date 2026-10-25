#include "InstanceDataReader.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/CSVReader.h"
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/InputEncoder.h"
#include <algorithm>
#include <ranges>
#include <stdexcept>

using namespace BPMNOS::Execution;

InstanceDataReader::InstanceDataReader(const BPMNOS::Model::Model* model, const std::string& instanceFileOrString, const std::vector<std::string>& columns)
  : model(model)
{
  enum { INSTANCE, NODE, INITIALIZATION, OTHER };
  if ( columns.size() < OTHER ) {
    throw std::logic_error("InstanceDataReader: at least three columns are required");
  }

  // a cell may hold an object initializer, which keeps its text
  BPMNOS::CSVReader reader(instanceFileOrString, ";", true);
  auto table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("InstanceDataReader: table '" + instanceFileOrString + "' is empty");
  }

  // the table names the first three or more of the columns given
  auto& header = table.front();
  if ( header.size() < OTHER || header.size() > columns.size() ) {
    throw std::runtime_error("InstanceDataReader: expected between " + std::to_string((size_t)OTHER) + " and " + std::to_string(columns.size()) + " columns, got " + std::to_string(header.size()));
  }
  for ( size_t i = 0; i < header.size(); i++ ) {
    if ( !std::holds_alternative<std::string>(header[i]) || std::get<std::string>(header[i]) != columns[i] ) {
      throw std::runtime_error("InstanceDataReader: column " + std::to_string(i + 1) + " must be named '" + columns[i] + "'");
    }
  }

  for ( auto& cells : table | std::views::drop(1) ) {
    if ( cells.empty() ) {
      continue;
    }
    if ( cells.size() != header.size() ) {
      throw std::runtime_error("InstanceDataReader: inconsistent number of cells");
    }
    for ( size_t i = 0; i < OTHER; i++ ) {
      if ( !std::holds_alternative<std::string>(cells[i]) ) {
        throw std::runtime_error("InstanceDataReader: illegal cell in column '" + columns[i] + "'");
      }
    }
    auto& instanceIdentifier = std::get<std::string>(cells[INSTANCE]);
    auto& nodeId = std::get<std::string>(cells[NODE]);

    Row row{0, nullptr, std::get<std::string>(cells[INITIALIZATION]), std::vector<std::string>(columns.size() - OTHER)};
    for ( size_t i = OTHER; i < cells.size(); i++ ) {
      row.cells[i - OTHER] = std::holds_alternative<std::string>(cells[i]) ?
        std::get<std::string>(cells[i]) :
        std::to_string((double)std::get<BPMNOS::number>(cells[i]));
    }

    if ( !row.initialization.empty() && !row.initialization.contains(":=") ) {
      // a size declaration, which holds for every instance
      if ( !instanceIdentifier.empty() ) {
        throw std::runtime_error("InstanceDataReader: size declaration '" + row.initialization + "' must not name an instance");
      }
      for ( size_t i = OTHER; i < cells.size(); i++ ) {
        if ( !row.cells[i - OTHER].empty() ) {
          throw std::runtime_error("InstanceDataReader: size declaration '" + row.initialization + "' must not have a value in column '" + columns[i] + "'");
        }
      }
      row.declaresSizes = true;
      row.node = nodeId.empty() ? nullptr : findNode(nodeId);
      rows.push_back(std::move(row));
      continue;
    }
    if ( instanceIdentifier.empty() && nodeId.empty() ) {
      // the value of a global attribute
      for ( size_t i = OTHER; i < cells.size(); i++ ) {
        if ( !row.cells[i - OTHER].empty() ) {
          throw std::runtime_error("InstanceDataReader: a global attribute must not have a value in column '" + columns[i] + "'");
        }
      }
      if ( !row.initialization.empty() ) {
        rows.push_back(std::move(row));
      }
      continue;
    }
    if ( instanceIdentifier.empty() ) {
      throw std::runtime_error("InstanceDataReader: instance required for node '" + nodeId + "'");
    }

    row.instanceId = (size_t)BPMNOS::to_number(instanceIdentifier, STRING);
    row.node = findNode(nodeId);
    if ( !processes.contains(row.instanceId) ) {
      // the first row of an instance names its process
      auto process = row.node->represents<BPMN::Process>();
      if ( !process ) {
        throw std::runtime_error("InstanceDataReader: the first row of instance '" + instanceIdentifier + "' must name a process, not '" + nodeId + "'");
      }
      processes[row.instanceId] = process;
    }
    rows.push_back(std::move(row));
  }
}

const BPMN::Node* InstanceDataReader::findNode(const std::string& nodeId) const {
  for ( auto& process : model->processes ) {
    if ( process->id == nodeId ) {
      return process.get();
    }
    if ( auto node = process->find([&nodeId](BPMN::Node* candidate) { return candidate->id == nodeId; }) ) {
      return node;
    }
  }
  throw std::runtime_error("InstanceDataReader: node '" + nodeId + "' not found in model");
}

std::pair<std::string, std::string> InstanceDataReader::splitInitialization(const std::string& initialization) {
  auto position = initialization.find(":=");
  if ( position == std::string::npos ) {
    throw std::runtime_error("InstanceDataReader: initialization must have the form 'attribute := expression', got '" + initialization + "'");
  }
  auto trim = [&initialization](std::string text) {
    auto start = text.find_first_not_of(" \t");
    if ( start == std::string::npos ) {
      throw std::runtime_error("InstanceDataReader: incomplete initialization '" + initialization + "'");
    }
    auto end = text.find_last_not_of(" \t");
    return text.substr(start, end - start + 1);
  };
  return { trim(initialization.substr(0, position)), trim(initialization.substr(position + 2)) };
}

std::pair<const BPMNOS::Model::Attribute*, std::string> InstanceDataReader::lookupAttribute(const BPMN::Node* node, const std::string& initialization) const {
  // the engine creates event subprocesses itself, with the values the model assigns to their attributes, so the
  // data may give them none
  if ( node->represents<BPMN::EventSubProcess>() ) {
    throw std::runtime_error("InstanceDataReader: no values may be given for event subprocess '" + node->id + "'");
  }

  auto [attributeName, expression] = splitInitialization(initialization);
  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( !extensionElements->attributeRegistry.contains(attributeName) ) {
    // the attributes a guidance declares take only the values the model assigns to them
    for ( auto guidance : { &extensionElements->entryGuidance, &extensionElements->exitGuidance, &extensionElements->choiceGuidance, &extensionElements->messageDeliveryGuidance } ) {
      if ( guidance->has_value() && guidance->value()->attributeRegistry.contains(attributeName) ) {
        throw std::runtime_error("InstanceDataReader: attribute '" + attributeName + "' of node '" + node->id + "' belongs to a guidance and takes no value from the data");
      }
    }
    throw std::runtime_error("InstanceDataReader: node '" + node->id + "' has no attribute '" + attributeName + "'");
  }

  auto attribute = extensionElements->attributeRegistry[attributeName];
  if ( !std::holds_alternative<std::monostate>(attribute->initialization) ) {
    throw std::runtime_error("InstanceDataReader: attribute '" + attributeName + "' is assigned by the model and must not be given");
  }
  return {attribute, expression};
}

namespace {

/// Returns the object a node declares with the given name, or the global object if no node is given.
const BPMNOS::Model::Attribute* findObject(const BPMNOS::Model::Model* model, const BPMN::Node* node, const std::string& name) {
  if ( !node ) {
    for ( auto& object : model->objects ) {
      if ( object->name == name ) {
        return object.get();
      }
    }
    return nullptr;
  }
  auto extensionElements = node->extensionElements ? node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() : nullptr;
  if ( !extensionElements ) {
    return nullptr;
  }
  for ( auto objects : { &extensionElements->statusObjects, &extensionElements->dataObjects } ) {
    for ( auto& object : *objects ) {
      if ( object->name == name ) {
        return object.get();
      }
    }
  }
  return nullptr;
}

} // namespace

bool InstanceDataReader::assignsObject(const Row& row) const {
  auto [name, _] = splitInitialization(row.initialization);
  if ( !row.node ) {
    return findObject(model, nullptr, name) != nullptr;
  }
  auto extensionElements = row.node->extensionElements ? row.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() : nullptr;
  return extensionElements && extensionElements->attributeRegistry.contains(name) && extensionElements->attributeRegistry[name]->isObject();
}

std::pair<const BPMNOS::Model::Attribute*, BPMNOS::ObjectInitializer> InstanceDataReader::lookupObject(const Row& row) const {
  auto [name, text] = splitInitialization(row.initialization);
  const BPMNOS::Model::Attribute* object = nullptr;
  if ( !row.node ) {
    object = findObject(model, nullptr, name);
    if ( !std::holds_alternative<std::monostate>(object->initialization) ) {
      throw std::runtime_error("InstanceDataReader: global object '" + name + "' is assigned by the model and must not be given");
    }
  }
  else {
    object = lookupAttribute(row.node, row.initialization).first;
  }

  // a literal without expression members has been registered as a constant object when the line was read, and
  // stands as its index; an object initializer stands as it is written
  try {
    size_t consumed = 0;
    size_t index = std::stoul(text, &consumed);
    if ( consumed == text.size() ) {
      return { object, BPMNOS::ObjectInitializer(index) };
    }
  }
  catch ( const std::exception& ) {
  }
  try {
    return { object, BPMNOS::ObjectInitializer(text) };
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("InstanceDataReader: object '" + name + "' must be given a literal, not '" + text + "'.\n" + error.what());
  }
}

void InstanceDataReader::setObject(const Row& row, const LIMEX::Handle<double>& handle) {
  auto [object, initializer] = lookupObject(row);
  std::optional<size_t> instanceId = row.node ? std::optional<size_t>(row.instanceId) : std::nullopt;
  // the members read what earlier rows have given, the object being recorded only when all its members are evaluated
  std::vector<double> values;
  for ( auto& member : initializer.members() ) {
    values.push_back( execute(instanceId, row.node, member.expression, handle) );
  }
  auto value = instantiate(getSchema(object), object, initializer, values);
  if ( instanceId.has_value() ) {
    objects[instanceId.value()][object] = std::move(value);
  }
  else {
    globalObjects[object] = std::move(value);
  }
}

namespace {

/// Returns the value type a schema gives at the path of a member of a literal, nullopt if the path does not fit the
/// schema.
std::optional<BPMNOS::ValueType> scalarAt(const BPMNOS::Schema& schema, const BPMNOS::ObjectInitializer::Path& path) {
  const BPMNOS::Schema* current = &schema;
  for ( auto& step : path ) {
    if ( step.empty() ) {
      // an element of an array
      auto array = std::get_if<BPMNOS::ArrayType>(current);
      if ( !array ) {
        return std::nullopt;
      }
      current = array->type.get();
      continue;
    }
    // a field of an object
    auto object = std::get_if<BPMNOS::ObjectType>(current);
    if ( !object ) {
      return std::nullopt;
    }
    auto field = std::ranges::find_if(object->fields, [&step](auto& candidate) { return candidate.name == step; });
    if ( field == object->fields.end() ) {
      return std::nullopt;
    }
    current = field->type.get();
  }
  if ( auto valueType = std::get_if<BPMNOS::ValueType>(current) ) {
    return *valueType;
  }
  return std::nullopt;
}

} // namespace

std::shared_ptr<const BPMNOS::Object> InstanceDataReader::instantiate(const BPMNOS::Schema& schema, const BPMNOS::Model::Attribute* object, const BPMNOS::ObjectInitializer& initializer, const std::vector<double>& values) {
  // a member is a single value cast to the value type the schema states at its position; a member whose position the
  // schema lacks is taken as a decimal and refused when the object is converted
  std::vector<std::pair<double, ValueType>> typedValues;
  auto& members = initializer.members();
  for ( size_t k = 0; k < members.size(); k++ ) {
    auto type = scalarAt(schema, members[k].path).value_or(ValueType::DECIMAL);
    auto value = (double)BPMNOS::cast(values[k], type);
    typedValues.emplace_back( value, type );
  }
  try {
    auto value = initializer.instantiate(typedValues);
    return schema.supports(*value->layout->schema) ? value : std::make_shared<const BPMNOS::Object>(schema, *value);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("InstanceDataReader: illegal value of object '" + object->name + "'.\n" + error.what());
  }
}

void InstanceDataReader::declareSizes(const Row& row) {
  auto& text = row.initialization;
  size_t end = 0;
  while ( end < text.size() && ( std::isalnum((unsigned char)text[end]) || text[end] == '_' ) ) {
    ++end;
  }
  auto name = text.substr(0, end);
  auto declare = [&text,end](BPMNOS::Schema& schema) {
    try {
      schema = schema.declareSizes( text.substr(end) );
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("InstanceDataReader: illegal size declaration '" + text + "'.\n" + error.what());
    }
  };

  if ( auto object = findObject(model, row.node, name) ) {
    declare( schemas.try_emplace(object, object->type).first->second );
    return;
  }

  if ( !row.node ) {
    // the content of a signal is declared by the name of the signal
    for ( auto& process : model->processes ) {
      for ( auto node : process->find_all([](const BPMN::Node* node) { return node->represents<BPMN::SignalThrowEvent>() || node->represents<BPMN::SignalCatchEvent>(); }) ) {
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if ( extensionElements->content->name == name ) {
          declare( signalSchemas.try_emplace(extensionElements->signalDefinition->name, extensionElements->content->type).first->second );
          return;
        }
      }
    }
    throw std::runtime_error("InstanceDataReader: size declaration '" + text + "' names no object or signal of the model");
  }

  // the content of a message, error or escalation is declared at the node throwing it
  auto extensionElements = row.node->extensionElements ? row.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() : nullptr;
  if ( !extensionElements || !extensionElements->content || extensionElements->content->name != name ) {
    throw std::runtime_error("InstanceDataReader: size declaration '" + text + "' names no object of node '" + row.node->id + "'");
  }
  if ( row.node->represents<BPMN::SignalThrowEvent>() || row.node->represents<BPMN::SignalCatchEvent>() ) {
    throw std::runtime_error("InstanceDataReader: size declaration '" + text + "' at node '" + row.node->id + "' must name no node, the sizes of a signal being declared by its name");
  }
  if ( !row.node->represents<BPMN::ThrowEvent>() ) {
    throw std::runtime_error("InstanceDataReader: size declaration '" + text + "' at node '" + row.node->id + "' must be given at the node throwing the content");
  }
  declare( schemas.try_emplace(extensionElements->content.get(), extensionElements->content->type).first->second );
}

BPMNOS::Schema InstanceDataReader::getSignalSchema(BPMNOS::number name, const BPMNOS::Schema& type) const {
  if ( auto it = signalSchemas.find(name); it != signalSchemas.end() ) {
    return it->second;
  }
  return type;
}

std::shared_ptr<const BPMNOS::Object> InstanceDataReader::knownObject(std::optional<size_t> instanceId, const BPMNOS::Model::Attribute* object) const {
  if ( instanceId.has_value() ) {
    if ( auto instanceObjects = objects.find(instanceId.value()); instanceObjects != objects.end() ) {
      if ( auto given = instanceObjects->second.find(object); given != instanceObjects->second.end() ) {
        return given->second;
      }
    }
  }
  else if ( auto given = globalObjects.find(object); given != globalObjects.end() ) {
    return given->second;
  }
  auto literal = std::get_if<size_t>(&object->initialization);
  if ( !literal ) {
    return nullptr;
  }
  try {
    auto schema = getSchema(object);
    auto& constant = objectRegistry[*literal];
    return schema.supports(*constant->layout->schema) ? constant : std::make_shared<const BPMNOS::Object>(schema, *constant);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("InstanceDataReader: illegal value of object '" + object->name + "'.\n" + error.what());
  }
}

BPMNOS::Schema InstanceDataReader::getSchema(const BPMNOS::Model::Attribute* object) const {
  if ( auto it = schemas.find(object); it != schemas.end() ) {
    return it->second;
  }
  return object->type;
}

void InstanceDataReader::evaluateGlobal(const std::string& initialization, const LIMEX::Handle<double>& handle) {
  auto [attributeName, expressionString] = splitInitialization(initialization);

  const BPMNOS::Model::Attribute* attribute = nullptr;
  for ( auto& globalAttribute : model->attributes ) {
    if ( globalAttribute->name == attributeName ) {
      attribute = globalAttribute.get();
      break;
    }
  }
  if ( !attribute ) {
    throw std::runtime_error("InstanceDataReader: unknown global attribute '" + attributeName + "'");
  }
  if ( !std::holds_alternative<std::monostate>(attribute->initialization) ) {
    throw std::runtime_error("InstanceDataReader: global attribute '" + attributeName + "' is assigned by the model and must not be given");
  }

  globals[attribute] = BPMNOS::cast(execute(std::nullopt, nullptr, expressionString, handle), std::get<BPMNOS::ValueType>(attribute->type));
}

BPMNOS::number InstanceDataReader::evaluate(size_t instanceId, const BPMN::Node* node, const std::string& expressionString, ValueType type, const LIMEX::Handle<double>& handle) const {
  return BPMNOS::cast(execute(instanceId, node, expressionString, handle), type);
}

double InstanceDataReader::execute(std::optional<size_t> instanceId, const BPMN::Node* node, const std::string& expressionString, const LIMEX::Handle<double>& handle) const {
  auto& registry = node ? node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry : model->attributeRegistry;
  BPMNOS::Model::Expression expression(handle, BPMNOS::InputEncoder::fragment(expressionString), registry);

  BPMNOS::Status status(registry.statusAttributes.size());
  BPMNOS::Data data(registry.dataAttributes.size());
  // the global attributes are the first data attributes
  for ( auto& [attribute, value] : globals ) {
    data.attributes[attribute->index] = value;
  }
  auto instanceValues = instanceId.has_value() ? values.find(instanceId.value()) : values.end();
  if ( instanceValues != values.end() ) {
    for ( auto& [attribute, value] : instanceValues->second ) {
      if ( attribute->category == BPMNOS::Model::Attribute::Category::STATUS ) {
        status.attributes[attribute->index] = value;
      }
      else if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        data.attributes[attribute->index] = value;
      }
    }
  }

  for ( auto attribute : expression.variables ) {
    bool known = attribute->isGlobal ?
      globals.contains(attribute) :
      ( instanceValues != values.end() && instanceValues->second.contains(attribute) );
    if ( !known ) {
      throw std::runtime_error("InstanceDataReader: expression '" + expressionString + "' refers to attribute '" + attribute->name + "' without a value");
    }
  }

  // the objects given by earlier rows or by the model, the global objects being the first data objects
  status.objects.resize(registry.statusObjects.size());
  for ( auto object : registry.statusObjects ) {
    status.objects[object->index] = knownObject(instanceId, object);
  }
  data.objects.resize(registry.dataObjects.size());
  for ( auto object : registry.dataObjects ) {
    bool global = ( object->index < model->objects.size() );
    data.objects[object->index] = knownObject(global ? std::nullopt : instanceId, object);
  }
  for ( auto attribute : expression.paths ) {
    // a path to an input has no attribute, an input holding its object from the start
    if ( attribute && !registry.getObject(attribute, status, data) ) {
      throw std::runtime_error("InstanceDataReader: expression '" + expressionString + "' refers to object '" + attribute->name + "' without a value");
    }
  }

  auto value = expression.execute(status, data);
  if ( !value.has_value() ) {
    throw std::runtime_error("InstanceDataReader: failed to evaluate expression '" + expressionString + "'");
  }
  return value.value();
}

void InstanceDataReader::setValue(size_t instanceId, const BPMNOS::Model::Attribute* attribute, BPMNOS::number value) {
  values[instanceId][attribute] = value;
}

void InstanceDataReader::addDefaultValues(size_t instanceId, std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& instanceValues) const {
  auto extensionElements = processes.at(instanceId)->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto instanceAttribute = extensionElements->data[BPMNOS::Model::ExtensionElements::Position::Instance].get();
  auto timestampAttribute = extensionElements->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get();
  for ( auto [attribute, value] : { std::pair{instanceAttribute, BPMNOS::number(instanceId)}, std::pair{timestampAttribute, BPMNOS::number(0)} } ) {
    if ( !instanceValues.contains(attribute) ) {
      if ( !std::holds_alternative<std::monostate>(attribute->initialization) ) {
        throw std::runtime_error("InstanceDataReader: the default attribute '" + attribute->name + "' must not be assigned by the model");
      }
      instanceValues[attribute] = value;
    }
  }
}

