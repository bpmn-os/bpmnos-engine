#include "InstanceDataReader.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/CSVReader.h"
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/InputEncoder.h"
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

  BPMNOS::CSVReader reader(instanceFileOrString, ";");
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
  // the engine creates event subprocesses and compensation activities itself, with the values the model
  // assigns to their attributes, so the data may give them none
  auto activity = node->represents<BPMN::Activity>();
  if ( node->represents<BPMN::EventSubProcess>() || ( activity && activity->isForCompensation ) ) {
    throw std::runtime_error("InstanceDataReader: no values may be given for event subprocess or compensation activity '" + node->id + "'");
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
  if ( attribute->expression || attribute->initialObject.has_value() ) {
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

void InstanceDataReader::setObject(const Row& row) {
  for ( auto& cell : row.cells ) {
    if ( !cell.empty() ) {
      throw std::runtime_error("InstanceDataReader: the value of an object is known from the start and takes no further value, in '" + row.initialization + "'");
    }
  }
  auto [name, literal] = splitInitialization(row.initialization);
  // the literal has been registered as a constant object when the line was read, and stands as its index
  size_t index = 0;
  try {
    index = std::stoul(literal);
    objectRegistry[index];
  }
  catch ( const std::exception& ) {
    throw std::runtime_error("InstanceDataReader: object '" + name + "' must be given a literal, not '" + literal + "'");
  }
  if ( !row.node ) {
    auto object = findObject(model, nullptr, name);
    if ( object->initialObject.has_value() || object->expression ) {
      throw std::runtime_error("InstanceDataReader: global object '" + name + "' is assigned by the model and must not be given");
    }
    globalObjects[object] = index;
    return;
  }
  auto [object, _] = lookupAttribute(row.node, row.initialization);
  objects[row.instanceId][object] = index;
}

void InstanceDataReader::declareSizes(const Row& row) {
  auto& text = row.initialization;
  size_t end = 0;
  while ( end < text.size() && ( std::isalnum((unsigned char)text[end]) || text[end] == '_' ) ) {
    ++end;
  }
  auto name = text.substr(0, end);
  auto object = findObject(model, row.node, name);
  if ( !object ) {
    throw std::runtime_error("InstanceDataReader: size declaration '" + text + "' names no object " + ( row.node ? "of node '" + row.node->id + "'" : std::string("of the model") ));
  }
  auto [it, _] = schemas.try_emplace(object, *object->schema);
  try {
    it->second.declareSizes( text.substr(end) );
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("InstanceDataReader: illegal size declaration '" + text + "'.\n" + error.what());
  }
}

std::shared_ptr<const BPMNOS::Object> InstanceDataReader::knownObject(std::optional<size_t> instanceId, const BPMNOS::Model::Attribute* object) const {
  auto index = object->initialObject;
  if ( instanceId.has_value() ) {
    if ( auto instanceObjects = objects.find(instanceId.value()); instanceObjects != objects.end() ) {
      if ( auto given = instanceObjects->second.find(object); given != instanceObjects->second.end() ) {
        index = given->second;
      }
    }
  }
  else if ( auto given = globalObjects.find(object); given != globalObjects.end() ) {
    index = given->second;
  }
  if ( !index.has_value() ) {
    return nullptr;
  }
  try {
    return getSchema(object).conform( objectRegistry[index.value()] );
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("InstanceDataReader: illegal value of object '" + object->name + "'.\n" + error.what());
  }
}

BPMNOS::Model::Schema InstanceDataReader::getSchema(const BPMNOS::Model::Attribute* object) const {
  if ( auto it = schemas.find(object); it != schemas.end() ) {
    return it->second;
  }
  return *object->schema;
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
  if ( attribute->expression ) {
    throw std::runtime_error("InstanceDataReader: global attribute '" + attributeName + "' is assigned by the model and must not be given");
  }

  BPMNOS::Model::Expression expression(handle, BPMNOS::InputEncoder::fragment(expressionString), model->attributeRegistry);
  // the global attributes are the first data attributes
  BPMNOS::Data globalValues(model->attributes.size());
  for ( auto& [globalAttribute, value] : globals ) {
    globalValues.attributes[globalAttribute->index] = value;
  }
  for ( auto referencedAttribute : expression.variables ) {
    if ( !globals.contains(referencedAttribute) ) {
      throw std::runtime_error("InstanceDataReader: global attribute '" + attributeName + "' refers to global attribute '" + referencedAttribute->name + "' without a value");
    }
  }
  // the global objects given by earlier rows or by the model
  for ( auto& object : model->objects ) {
    globalValues.objects.push_back( knownObject(std::nullopt, object.get()) );
  }
  for ( auto referencedAttribute : expression.paths ) {
    if ( !globalValues.objects[referencedAttribute->index] ) {
      throw std::runtime_error("InstanceDataReader: global attribute '" + attributeName + "' refers to global object '" + referencedAttribute->name + "' without a value");
    }
  }

  auto value = expression.execute(BPMNOS::Status{}, globalValues);
  if ( !value.has_value() ) {
    throw std::runtime_error("InstanceDataReader: failed to evaluate global attribute '" + attributeName + "'");
  }
  globals[attribute] = convert(value.value(), attribute->type);
}

BPMNOS::number InstanceDataReader::evaluate(size_t instanceId, const BPMN::Node* node, const std::string& expressionString, ValueType type, const LIMEX::Handle<double>& handle) const {
  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  BPMNOS::Model::Expression expression(handle, BPMNOS::InputEncoder::fragment(expressionString), extensionElements->attributeRegistry);

  BPMNOS::Status status(extensionElements->attributeRegistry.statusAttributes.size());
  BPMNOS::Data data(extensionElements->attributeRegistry.dataAttributes.size());
  // the global attributes are the first data attributes
  for ( auto& [attribute, value] : globals ) {
    data.attributes[attribute->index] = value;
  }
  auto instanceValues = values.find(instanceId);
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
  auto& registry = extensionElements->attributeRegistry;
  status.objects.resize(registry.statusObjects.size());
  for ( auto object : registry.statusObjects ) {
    status.objects[object->index] = knownObject(instanceId, object);
  }
  data.objects.resize(registry.dataObjects.size());
  for ( auto object : registry.dataObjects ) {
    bool global = ( object->index < model->objects.size() );
    data.objects[object->index] = knownObject(global ? std::nullopt : std::optional<size_t>(instanceId), object);
  }
  for ( auto attribute : expression.paths ) {
    if ( !registry.getObject(attribute, status, data) ) {
      throw std::runtime_error("InstanceDataReader: expression '" + expressionString + "' refers to object '" + attribute->name + "' without a value");
    }
  }

  auto value = expression.execute(status, data);
  if ( !value.has_value() ) {
    throw std::runtime_error("InstanceDataReader: failed to evaluate expression '" + expressionString + "'");
  }
  return convert(value.value(), type);
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
      if ( attribute->expression ) {
        throw std::runtime_error("InstanceDataReader: the default attribute '" + attribute->name + "' must not be assigned by the model");
      }
      instanceValues[attribute] = value;
    }
  }
}

BPMNOS::number InstanceDataReader::convert(double value, ValueType type) {
  switch ( type ) {
    case ValueType::INTEGER:
      return BPMNOS::number((int)value);
    case ValueType::BOOLEAN:
      return BPMNOS::number(value != 0 ? 1 : 0);
    default:
      return BPMNOS::number(value);
  }
}
