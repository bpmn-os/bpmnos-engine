#ifndef BPMNOS_Model_StaticDataProvider_H
#define BPMNOS_Model_StaticDataProvider_H

#include "DataProvider.h"
#include "model/utility/src/CSVReader.h"
#include <vector>

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
  /**
   * @brief Constructor for StaticDataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param folders The folders containing lookup tables.
   * @param instanceFileOrString The file path to the instance data file or a string containing the data.
   */
  StaticDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString);
  ~StaticDataProvider() override = default;
  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;
protected:
  CSVReader reader;
  void readInstances();
  struct StaticInstanceData {
    const BPMN::Process* process;
    BPMNOS::number id;
    BPMNOS::number instantiation;
    std::unordered_map< const Attribute*, BPMNOS::number > data;
  };
  std::unordered_map< long unsigned int, StaticInstanceData > instances;
  std::unordered_map< const Attribute*, BPMNOS::number > globalValueMap;
  BPMNOS::number earliestInstantiation;
  BPMNOS::number latestInstantiation;
  void ensureDefaultValue(StaticInstanceData& instance, const std::string attributeId, std::optional<BPMNOS::number> value = std::nullopt);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StaticDataProvider_H
