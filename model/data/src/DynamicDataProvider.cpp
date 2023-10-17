#include "DynamicDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Status.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>


using namespace BPMNOS::Model;

DynamicDataProvider::DynamicDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( initReader(instanceFileOrString) )
{
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
      // instance attribute is known at time zero, even if instantiation is disclosed later
      instance.data[ instanceAtribute ] = {{0,BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING)}};
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
    instance.data[ attribute ].push_back({ BPMNOS::to_number(row[DISCLOSURE].get(),INTEGER), BPMNOS::to_number(row[VALUE].get(),attribute->type) });

    if ( attribute->index == Status::Index::Timestamp ) {
      instance.instantiation.push_back({ BPMNOS::to_number(row[DISCLOSURE].get(),INTEGER), BPMNOS::to_number(row[VALUE].get(),attribute->type) });
    }

  }
}

std::unique_ptr<Scenario> DynamicDataProvider::createScenario(unsigned int scenarioId) {
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), attributes, scenarioId);
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
        // no attribute data given, use default value as realization at time 0
        scenario->setRealization( scenario->getAttributeData(id, attribute), {0, attribute->value} );
      }

    }
  }
  return scenario;
}
