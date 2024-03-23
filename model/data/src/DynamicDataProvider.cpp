#include "DynamicDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>


using namespace BPMNOS::Model;

DynamicDataProvider::DynamicDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( initReader(instanceFileOrString) )
{
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
  readInstances();
}

csv::CSVReader DynamicDataProvider::initReader(const std::string& instanceFileOrString) {
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

void DynamicDataProvider::readInstances() {
  enum {PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE, DISCLOSURE};

  for (auto &row : reader) {
    std::string processId = row[PROCESS_ID].get();
    // find process with respective identifier
    auto processIt = std::find_if(
      model->processes.begin(),
      model->processes.end(),
      [&processId](const std::unique_ptr<BPMN::Process>& process) { return process->id == processId;}
    );
    if ( processIt == model->processes.end() ) {
      throw std::runtime_error("DynamicDataProvider: model has no process '" + processId + "'");
    }

    auto process = processIt->get();

    std::string instanceId = row[INSTANCE_ID].get();
    // find instance with respective identifier
    if ( !instances.contains(instanceId) ) {
      // row has first entry for instance, create new entry in data
      instances[instanceId] = DynamicInstanceData({process,instanceId,{},{}});
    }

    auto& instance = instances[instanceId];
    auto instanceAtribute = attributes[process][Keyword::Instance];
    if ( auto it = instance.data.find( instanceAtribute );
         it == instance.data.end()
    ) {
      // instance attribute is assumed to be known, even if instantiation is disclosed later
      instance.data[ instanceAtribute ] = { { std::numeric_limits<BPMNOS::number>::min(), BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING) } };
    }

    std::string attributeId = row[ATTRIBUTE_ID].get();

    if ( attributeId == "" ) {
      // no attribute provided in this row
      continue;
    }

    if ( !attributes[process].contains(attributeId) ) {
      throw std::runtime_error("DynamicDataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
    }

    auto attribute = attributes[process][attributeId];
    BPMNOS::number disclosure = BPMNOS::to_number(row[DISCLOSURE].get(),INTEGER);
    BPMNOS::number value = BPMNOS::to_number(row[VALUE].get(),attribute->type);
    auto &attributeData = instance.data[ attribute ];
    if ( attributeData.size() && attributeData.back().first >= disclosure ) {
      throw std::runtime_error("DynamicDataProvider: disclosures of attribute values must be provided in strictly increasing order");
    }
    instance.data[ attribute ].push_back({ disclosure, value });

    if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      if ( instance.instantiation.size() && instance.instantiation.back().first >= disclosure ) {
        throw std::runtime_error("DynamicDataProvider: disclosures of timestamp values must be provided in strictly increasing order");
      }
      if ( instance.instantiation.size() && instance.instantiation.back().first >= instance.instantiation.back().second ) {
        throw std::runtime_error("DynamicDataProvider: disclosures of anticipated timestamps must be strictly smaller than the timestamp value");
      }
      instance.instantiation.push_back({ disclosure, value });
    }

  }
  for (auto& [id, instance] : instances) {
    auto& instantiation = instance.instantiation.back().second;

    if ( earliestInstantiation > instantiation ) {
      earliestInstantiation = instantiation;
    }
    if ( latestInstantiation < instantiation ) {
      latestInstantiation = instantiation;
    }
  }
}

std::unique_ptr<Scenario> DynamicDataProvider::createScenario(unsigned int scenarioId) {
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), earliestInstantiation, latestInstantiation, attributes, scenarioId);
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiation.size() ) {
      // instances become known at last disclosure of the instantiation
      auto& [instantiationDisclosure, instantiationTime] = instance.instantiation.back();
      scenario->addInstance(instance.process, id, { {}, {{instantiationDisclosure, instantiationTime}} });
      for ( auto& [disclosure, value] : instance.instantiation ) {
        scenario->addAnticipation( scenario->getInstantiationData(id), {disclosure, value} );
      }
    }

    for ( auto& [attribute, data] : instance.data ) {
      for ( auto& [disclosure, value] : data ) {
        // add anticipation of attribute value
        scenario->addAnticipation( scenario->getAttributeData(id, attribute), {disclosure, value} );
      }

      if ( data.size() ) {
        // use last given attribute data as realization
        auto& [disclosure, value] = data.back();
        scenario->setRealization( scenario->getAttributeData(id, attribute), {disclosure, value} );
      }
      else {
        // no attribute data given, use default value as realization at time
        scenario->setRealization( scenario->getAttributeData(id, attribute), {earliestInstantiation, attribute->value} );
      }

    }
  }
  return scenario;
}
