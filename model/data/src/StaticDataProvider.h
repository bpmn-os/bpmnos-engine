#ifndef BPMNOS_StaticDataProvider_H
#define BPMNOS_StaticDataProvider_H

#include "DataProvider.h"
#include <csv.hpp>

namespace BPMNOS::Model {

/**
 * @brief Class representing a data provider for static BPMN instance data.
 *
 * The StaticDataProvider class is responsible for providing and managing instance data
 * for BPMN processes. 
 * */
class StaticDataProvider : public DataProvider {
public:
  /**
   * @brief Constructor for StaticDataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param instanceFileOrString The file path to the instance data file or a string containing the data.
   */
  StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString);
  ~StaticDataProvider() override = default;
  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;
protected:
  csv::CSVReader initReader(const std::string& instanceFileOrString);
  csv::CSVReader reader;
  void readInstances();
  struct StaticInstanceData {
    const BPMN::Process* process;
    std::string id;
    std::unordered_map< const Attribute*, BPMNOS::number > data;
  };
  std::unordered_map< std::string, StaticInstanceData > instances;
  void ensureDefaultValue(StaticInstanceData& instance, const std::string attributeId, BPMNOS::number value);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_StaticDataProvider_H
