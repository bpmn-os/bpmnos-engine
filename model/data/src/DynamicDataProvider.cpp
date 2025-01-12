#include "DynamicDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <unordered_map>
#include <algorithm>
#include <ranges>


using namespace BPMNOS::Model;

DynamicDataProvider::DynamicDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( CSVReader(instanceFileOrString) )
{
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
  readInstances();
}

void DynamicDataProvider::readInstances() {
  enum {PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE, DISCLOSURE};

  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("DynamicDataProvider: table '" + reader.instanceFileOrString + "' is empty");
  }

  for (auto &row : table | std::views::drop(1)) {   // assume a single header line
    if ( row.empty() ) {
      continue;
    }
    if ( row.size() != 5 ) {
      throw std::runtime_error("DynamicDataProvider: illegal number of cells");
    }
/*
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

    // TODO: populate globalValueMap!!!
    auto process = processIt->get();

    auto instanceId = (long unsigned int)BPMNOS::to_number( row[INSTANCE_ID].get(), STRING );
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
      instance.data[ instanceAtribute ] = { { std::numeric_limits<BPMNOS::number>::min(), instanceId } };
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
*/
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
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), earliestInstantiation, latestInstantiation, attributes, globalValueMap, scenarioId);
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
// TODO
//        scenario->setRealization( scenario->getAttributeData(id, attribute), {earliestInstantiation, attribute->value} );
      }

    }
  }
  return scenario;
}
