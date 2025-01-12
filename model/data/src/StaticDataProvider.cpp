#include "StaticDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/getDelimiter.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <unordered_map>
#include <algorithm>
#include <ranges>

using namespace BPMNOS::Model;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DataProvider(modelFile)
  , reader( CSVReader(instanceFileOrString) )
{
  for ( auto& [ attributeId, attribute ] : attributes[nullptr] ) {
    if ( attribute->expression ) {
      if ( attribute->expression->compiled.getVariables().size() || attribute->expression->compiled.getCollections().size() ) {
        throw std::runtime_error("StaticDataProvider: initial value of global attribute '" + attribute->id + "' must not be derived from other attributes");
      }
      globalValueMap[ attribute ] = attribute->expression->compiled.evaluate();
    }
  }
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
  readInstances();
  
}

void StaticDataProvider::readInstances() {
  enum {PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE};

  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("StaticDataProvider: table '" + reader.instanceFileOrString + "' is empty");
  }

  for (auto &row : table | std::views::drop(1)) {   // assume a single header line
    if ( row.empty() ) {
      continue;
    }
    if ( row.size() != 4 ) {
      throw std::runtime_error("StaticDataProvider: illegal number of cells");
    }
    if ( !std::holds_alternative<std::string>(row.at(PROCESS_ID)) ) {
      throw std::runtime_error("StaticDataProvider: illegal process id");
    }
    std::string processId = std::get<std::string>(row.at(PROCESS_ID));

    if ( processId.size() ) {
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

      if ( !std::holds_alternative<std::string>(row.at(INSTANCE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal instance id");
      }
      auto instanceId = (size_t)BPMNOS::to_number(std::get<std::string>(row.at(INSTANCE_ID)), STRING );
//      auto instanceId = (long unsigned int)BPMNOS::to_number( row[INSTANCE_ID].get(), STRING );
      // find instance with respective identifier
      if ( !instances.contains(instanceId) ) {
        // row has first entry for instance, create new entry in data
        instances[instanceId] = StaticInstanceData({process,instanceId,std::numeric_limits<BPMNOS::number>::max(),{}});
      }

      auto& instance = instances[instanceId];

      if ( !std::holds_alternative<std::string>(row.at(ATTRIBUTE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal attribute id");
      }
      std::string attributeId = std::get<std::string>(row.at(ATTRIBUTE_ID));
//      std::string attributeId = row[ATTRIBUTE_ID].get();

      if ( attributeId == "" ) {
        // no attribute provided in this row
        continue;
      }

      if ( !attributes[process].contains(attributeId) ) {
        throw std::runtime_error("StaticDataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
      }

      auto attribute = attributes[process][attributeId];
      if ( attribute->expression ) {
        throw std::runtime_error("StaticDataProvider: value of attribute '" + attributeId + "' is initialized by expression and must not be provided explicitly");
      }

      if ( !std::holds_alternative<BPMNOS::number>(row.at(VALUE)) ) {
        throw std::runtime_error("StaticDataProvider: illegal value");
      }
      instance.data[ attribute ] = std::get<BPMNOS::number>(row.at(VALUE)); //BPMNOS::to_number(row[VALUE].get(),attribute->type);
//std::cerr << processId << ", " << BPMNOS::to_string(instanceId,STRING) << ", " <<  attributeId << ", " <<  BPMNOS::to_string(instance.data[ attribute ],attribute->type) << std::endl;
    }
    else {
      // row contains global attribute
      if ( !std::holds_alternative<std::string>(row.at(ATTRIBUTE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal attribute id");
      }
      std::string attributeId = std::get<std::string>(row.at(ATTRIBUTE_ID));
//      std::string attributeId = row[ATTRIBUTE_ID].get();
      auto attribute = attributes[nullptr][attributeId];
      if ( !std::holds_alternative<BPMNOS::number>(row.at(VALUE)) ) {
        throw std::runtime_error("StaticDataProvider: illegal value");
      }
      globalValueMap[attribute] = std::get<BPMNOS::number>(row.at(VALUE)); //BPMNOS::to_number(row[VALUE].get(),attribute->type);
//std::cerr  << ", " << ", " <<  attributeId << ", " <<  BPMNOS::to_string(globalValueMap[attribute],attribute->type) << std::endl;
    }
  }

  for (auto& [id, instance] : instances) {
    // ensure that default attributes are available
    ensureDefaultValue(instance,Keyword::Instance, id);
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
  assert( attributes.contains(instance.process) );
  auto it1 = attributes.at(instance.process).find(attributeId);
  if ( it1 == attributes.at(instance.process).end() ) {
    throw std::runtime_error("StaticDataProvider: unable to find required attribute '" + attributeId + "' for process '" + instance.process->id + "'");
  }
  auto attribute = it1->second;
  if ( auto it2 = instance.data.find( attribute );
    it2 == instance.data.end()
  ) {
    if ( attribute->expression ) {
      throw std::runtime_error("StaticDataProvider: initial value of default attribute '" + attribute->id + "' must not be  provided by expression");
    }
    
    // set attribute value if available
    if ( value.has_value() ) {
      instance.data[ attribute ] = value.value();
    }
    else if ( attributeId == BPMNOS::Keyword::Timestamp ) {
      // use 0 as fallback 
      instance.data[ attribute ] = 0;
    }
    else {
      throw std::runtime_error("StaticDataProvider: attribute '" + attribute->id + "' has no default value");
    }
  }
}

std::unique_ptr<Scenario> StaticDataProvider::createScenario(unsigned int scenarioId) {
  std::unique_ptr<Scenario> scenario = std::make_unique<Scenario>(model.get(), earliestInstantiation, latestInstantiation, attributes, globalValueMap, scenarioId);
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
