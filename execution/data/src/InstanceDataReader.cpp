#include "InstanceDataReader.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/CSVReader.h"
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
  if ( attribute->expression ) {
    throw std::runtime_error("InstanceDataReader: attribute '" + attributeName + "' is assigned by the model and must not be given");
  }
  return {attribute, expression};
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
    bool known = ( attribute->category == BPMNOS::Model::Attribute::Category::DATA && attribute->index < model->instanceIndex ) ?
      globals.contains(attribute) :
      ( instanceValues != values.end() && instanceValues->second.contains(attribute) );
    if ( !known ) {
      throw std::runtime_error("InstanceDataReader: expression '" + expressionString + "' refers to attribute '" + attribute->name + "' without a value");
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
