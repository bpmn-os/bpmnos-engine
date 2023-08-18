#include "StaticDataProvider.h"
#include <csv.hpp>

using namespace BPMNOS;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceDataFile)
  : DataProvider(modelFile)
{
  csv::CSVFormat format;
  format.trim({' ', '\t'});
  csv::CSVReader reader = csv::CSVReader(instanceDataFile, format);

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

    auto& process = *processIt->get();

    std::string instanceId = row[INSTANCE_ID].get();
    // find instance with respective identifier
    if ( !instances.contains(instanceId) ) {
      // row has first entry for instance, create new entry in data
      instances[instanceId] = std::make_unique< StaticInstanceData >(process,instanceId);
    } 

    StaticInstanceData* instanceData = static_cast<StaticInstanceData*>(instances[instanceId].get());

    std::string attributeId = row[ATTRIBUTE_ID].get();

    if ( attributeId == "" ) {
      // no attribute provided in this row
      continue;
    }

    if ( !instanceData->attributes.contains(attributeId) ) {
      throw std::runtime_error("DataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
    }

    auto attribute = instanceData->attributes[attributeId];
 
    // Mimic XML attribute to have consistent type conversion
    XML::Attribute givenValue = {
      .xmlns=attribute->element->xmlns,
      .prefix=attribute->element->prefix,
      .name=attribute->element->name,
      .value = row[VALUE].get()
    };

    if ( attribute->type == Attribute::Type::STRING ) {
      instanceData->actualValues[ attribute ] = (std::string)givenValue;
    }
    else if ( attribute->type == Attribute::Type::BOOLEAN ) {
      instanceData->actualValues[ attribute ] = (bool)givenValue;
    }
    else if ( attribute->type == Attribute::Type::INTEGER ) {
      instanceData->actualValues[ attribute ] = (int)givenValue;
    }
    else if ( attribute->type == Attribute::Type::DECIMAL ) {
      instanceData->actualValues[ attribute ] = (double)givenValue;
    }
  }
}

