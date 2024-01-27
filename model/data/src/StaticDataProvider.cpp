#include "StaticDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>

using namespace BPMNOS::Model;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( initReader(instanceFileOrString) )
{
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
  readInstances();
}

csv::CSVReader StaticDataProvider::initReader(const std::string& instanceFileOrString) {
  csv::CSVFormat format;
  format.trim({' ', '\t'});
  if (instanceFileOrString.find(",") == std::string::npos) {
    return csv::CSVReader(instanceFileOrString, format);
  }
  else {
    std::stringstream is;
    is << instanceFileOrString;
    return csv::CSVReader(is, format);
  }
}

void StaticDataProvider::readInstances() {
  enum {PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE};

  for (auto &row : reader) {
    std::string processId = row[PROCESS_ID].get();
    // find process with respective identifier
    auto processIt = std::find_if(
      model->processes.begin(),
      model->processes.end(),
      [&processId](const std::unique_ptr<BPMN::Process>& process) { return process->id == processId;}
    );
    if ( processIt == model->processes.end() ) {
      throw std::runtime_error("StaticDataProvider: model has no process '" + processId + "'");
    }

    auto process = processIt->get();

    std::string instanceId = row[INSTANCE_ID].get();
    // find instance with respective identifier
    if ( !instances.contains(instanceId) ) {
      // row has first entry for instance, create new entry in data
      instances[instanceId] = StaticInstanceData({process,instanceId,std::numeric_limits<BPMNOS::number>::max(),{}});
    }

    auto& instance = instances[instanceId];

    std::string attributeId = row[ATTRIBUTE_ID].get();

    if ( attributeId == "" ) {
      // no attribute provided in this row
      continue;
    }

    if ( !attributes[process].contains(attributeId) ) {
      throw std::runtime_error("StaticDataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
    }

    auto attribute = attributes[process][attributeId];
    instance.data[ attribute ] = BPMNOS::to_number(row[VALUE].get(),attribute->type);
  }

  for (auto& [id, instance] : instances) {
    // ensure that default attributes are available
    ensureDefaultValue(instance,Keyword::Instance,BPMNOS::to_number(id,BPMNOS::ValueType::STRING));
    ensureDefaultValue(instance,Keyword::Timestamp);
    // set time of instantiation
    instance.instantiation = instance.data.at( attributes[instance.process][Keyword::Timestamp] );

    if ( earliestInstantiation > instance.instantiation ) {
      earliestInstantiation = instance.instantiation;
    }
    if ( latestInstantiation < instance.instantiation ) {
      latestInstantiation = instance.instantiation;
    }
  }
}

void StaticDataProvider::ensureDefaultValue(StaticInstanceData& instance, const std::string attributeId, std::optional<BPMNOS::number> value) {
  auto& attribute = attributes[instance.process][attributeId];
  if ( auto it = instance.data.find( attribute );
    it == instance.data.end()
  ) {
    // set attribute if not yet set
    if ( value.has_value() ) {
      instance.data[ attribute ] = value.value();
    }
    else if ( attribute->value.has_value() ) {
      instance.data[ attribute ] = attribute->value.value();
    }
    else {
      throw std::runtime_error("StaticDataProvider: attribute '" + attribute->id + "' has no default value");
    }
  }
}

std::unique_ptr<Scenario> StaticDataProvider::createScenario(unsigned int scenarioId) {
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), earliestInstantiation, latestInstantiation, attributes, scenarioId);
  for ( auto& [id, instance] : instances ) {
    auto& timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    auto& instantiation = instance.data[timestampAttribute];
    scenario->addInstance(instance.process, id, { {}, {{earliestInstantiation, instantiation}} }); // all instances are known at time of the earliest instantiation, but instantiations may occur later
    for ( auto& [attribute, value] : instance.data ) {
      scenario->setRealization( scenario->getAttributeData(id, attribute), {earliestInstantiation, value} ); // all attribute values are known at time of the earliest instantiation
    }
  }
  return scenario;
}
