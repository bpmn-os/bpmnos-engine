#ifndef BPMNOS_DataProvider_H
#define BPMNOS_DataProvider_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include <csv.hpp>
#include "model/parser/src/Model.h"
#include "InstanceData.h"

namespace BPMNOS {

/**
 * @brief Template class representing a data provider for BPMN instance data.
 *
 * The DataProvider class is responsible for providing and managing instance data
 * for BPMN processes. It reads instance data from a file and creates InstanceData
 * objects for each instance. The created instances are stored in a vector for
 * easy access and retrieval.
 *
 * @tparam T The numeric type used for the instance attribute values.
 */
template <typename T>
class DataProvider {
public:
/**
   * @brief Constructor for DataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param instanceDataFile The file path to the instance data file.
   */
  DataProvider(const std::string& modelFile, const std::string& instanceDataFile) : model(std::make_unique<Model>(modelFile)), instances(getInstances(instanceDataFile)) {};

  const std::unique_ptr<Model> model;  ///< Pointer to the BPMN model.
  std::vector< std::unique_ptr< InstanceData<T> > > instances; ///< A vector holding the data of all instances.

protected:
  /**
   * @brief Method for obtaining instance data from a data file.
   *
   * This method reads the instance data from a CSV file and creates InstanceData
   * objects for each instance. The created instances are stored in a vector for
   * easy access and retrieval.
   *
   * This method can be overwritten to use other data sources or to create other types
   * of instance data that are derived from InstanceData<T>.
   *
   * @param instanceDataFile The file path to the instance data file.
   * @return Vector of unique pointers to the created InstanceData objects.
   */
  virtual std::vector< std::unique_ptr< InstanceData<T> > > getInstances(const std::string& instanceDataFile) {
    std::vector< std::unique_ptr< InstanceData<T> > > data;

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
      auto instanceDataIt = std::find_if( 
        data.begin(), 
        data.end(),
        [&instanceId](const std::unique_ptr< InstanceData<T> >& instance) { return instance->id == instanceId;}
      );
      if ( instanceDataIt == data.end() ) {
        // row has first entry for instance, create new entry in data
        data.push_back(std::make_unique< InstanceData<T> >(process,instanceId));
        instanceDataIt = data.end() - 1;
      }

      auto& instanceData = *instanceDataIt->get();

      std::string attributeId = row[ATTRIBUTE_ID].get();

      if ( attributeId == "" ) {
        // no attribute provided in this row
        continue;
      }

      // find node for which the attribute is declared
      auto node = process.find(
        [&attributeId](const BPMN::Node* n) {
          if ( auto status = n->extensionElements->represents<Status>(); status ) {
            auto attributeIt =std::find_if(
              status->attributes.begin(), 
              status->attributes.end(),
              [&attributeId](const std::reference_wrapper<XML::bpmnos::tAttribute>& attribute) {
                return attribute.get().id.value == attributeId;
              }
            );
            return ( attributeIt != status->attributes.end() );
          }
          return false;
        }
      );
      if ( node == nullptr ) {
        throw std::runtime_error("DataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
      }

      std::string value = row[VALUE].get();
      instanceData.update(node, attributeId, value);

    }

    return data;
  };

  std::unordered_map<std::string, BPMN::Process*> processes; ///< Map of processes defined in the model.
};

} // namespace BPMNOS

#endif // BPMNOS_DataProvider_H
