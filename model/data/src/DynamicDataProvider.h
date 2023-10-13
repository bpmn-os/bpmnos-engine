#ifndef BPMNOS_DynamicDataProvider_H
#define BPMNOS_DynamicDataProvider_H

#include "DataProvider.h"
#include <csv.hpp>

namespace BPMNOS::Model {

/**
 * @brief Class representing a data provider for dynamic BPMN instance data.
 *
 * The DynamicDataProvider class is responsible for providing and managing instance data
 * for BPMN processes.
 * */
class DynamicDataProvider : public DataProvider {
public:
  /**
   * @brief Constructor for DynamicDataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param instanceFileOrString The file path to the instance data file or a string containing the data.
   */
  DynamicDataProvider(const std::string& modelFile, const std::string& instanceFileOrString);
  ~DynamicDataProvider() override = default;
  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;
protected:
  csv::CSVReader initReader(const std::string& instanceFileOrString);
  csv::CSVReader reader;
  void readInstances();

  struct DynamicInstanceData {
    const BPMN::Process* process;
    std::string instanceId;
    std::vector< std::pair<BPMNOS::number,BPMNOS::number> > instantiation; ///< Instantiation data consisting of disclosure-value pairs
    std::unordered_map< const Attribute*, std::vector< std::pair<BPMNOS::number,BPMNOS::number> > > data; ///< Map of attribute data consisting of disclosure-value pairs
  };
  std::unordered_map< std::string, DynamicInstanceData > instances;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_DynamicDataProvider_H