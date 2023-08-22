#include "StaticDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>

using namespace BPMNOS;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  :  DataProvider(modelFile)
{
  csv::CSVFormat format;
  format.trim({' ', '\t'});
  if (instanceFileOrString.find(",") == std::string::npos) {
    csv::CSVReader reader(instanceFileOrString, format);
    readInstances( reader );
  }
  else {
    std::stringstream is;
    is << instanceFileOrString;
    csv::CSVReader reader(is, format);
    readInstances( reader );
  }
}

void StaticDataProvider::readInstances(csv::CSVReader& reader) {
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
      throw std::runtime_error("DataProvider: model has no process '" + processId + "'");
    }

    auto process = processIt->get();

    std::string instanceId = row[INSTANCE_ID].get();
    // find instance with respective identifier
    if ( !instances.contains(instanceId) ) {
      // row has first entry for instance, create new entry in data
      instances[instanceId] = std::make_unique< StaticInstanceData >(process,instanceId);
    } 

    StaticInstanceData* instanceData = static_cast<StaticInstanceData*>(instances[instanceId].get());

    if ( auto attributeIt = instanceData->attributes.find(Keyword::Instance); attributeIt != instanceData->attributes.end() ) {
      // set instance attribute if not yet set
      if ( !instanceData->actualValues[ attributeIt->second ].has_value() ) {
        instanceData->actualValues[ attributeIt->second ] = to_number(instanceId,ValueType::STRING);
      }
    }

    std::string attributeId = row[ATTRIBUTE_ID].get();

    if ( attributeId == "" ) {
      // no attribute provided in this row
      continue;
    }

    if ( !instanceData->attributes.contains(attributeId) ) {
      throw std::runtime_error("DataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
    }

    auto attribute = instanceData->attributes[attributeId];
    instanceData->actualValues[ attribute ] = to_number(row[VALUE].get(),attribute->type);
  }
}

