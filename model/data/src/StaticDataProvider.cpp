#include "StaticDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>

using namespace BPMNOS::Model;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( initReader(instanceFileOrString) )
{
  readInstances();
  createScenario();
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
      instances[instanceId] = StaticInstanceData({process,instanceId,{}});
    } 

    auto& instance = instances[instanceId];
    if ( auto it = instance.data.find( attributes[process][Keyword::Instance] ); 
         it == instance.data.end() 
    ) {
      // set instance attribute if not yet set
      instance.data[ it->first ] = BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING);
    }

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
}

void StaticDataProvider::createScenario() {
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), attributes);
  for ( auto& [id, instance] : instances ) {
    scenario->addInstance(instance.process, id, { {}, {{0, 0}} }); // all instances are known at time 0
    for ( auto& [attribute, value] : instance.data ) {
      scenario->setRealization( scenario->getAttributeData(id, attribute), {0, value} ); // all attribute values are known at time 0
    }
  }
  scenarios.push_back(std::move(scenario));
}
