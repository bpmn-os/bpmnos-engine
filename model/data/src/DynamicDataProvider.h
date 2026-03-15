#ifndef BPMNOS_Model_DynamicDataProvider_H
#define BPMNOS_Model_DynamicDataProvider_H

#include <bpmn++.h>
#include "DataProvider.h"
#include "model/utility/src/CSVReader.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <memory>
#include <vector>

namespace BPMNOS::Model {

/**
 * @brief Structure representing a deferred initialization.
 *
 * Stores information needed to evaluate an initialization expression
 * at disclosure time rather than at parse time.
 */
struct DeferredInitialization {
  const Attribute* attribute;              ///< The attribute to initialize
  BPMNOS::number disclosureTime;           ///< Effective disclosure time (max of own, parent)
  std::unique_ptr<Expression> expression;  ///< Compiled expression to evaluate at disclosure time
};

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
  /**
   * @brief Constructor for DynamicDataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param folders The folders containing lookup tables.
   * @param instanceFileOrString The file path to the instance data file or a string containing the data.
   */
  DynamicDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString);
  ~DynamicDataProvider() override = default;
  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;
protected:
  CSVReader reader;
  void readInstances();

  struct DynamicInstanceData {
    const BPMN::Process* process;
    BPMNOS::number id;
    BPMNOS::number instantiation;
    std::unordered_map< const Attribute*, BPMNOS::number > data;
  };
  std::unordered_map< long unsigned int, DynamicInstanceData > instances;
  std::unordered_map< const Attribute*, BPMNOS::number > globalValueMap;
  std::unordered_map< size_t, std::vector<DeferredInitialization> > deferredInitializations; ///< Instance ID -> deferred inits
  std::unordered_map< size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number> > disclosure; ///< Instance ID -> Node -> time when node's data is disclosed
  BPMNOS::number earliestInstantiation;
  BPMNOS::number latestInstantiation;

  void ensureDefaultValue(DynamicInstanceData& instance, const std::string attributeId, std::optional<BPMNOS::number> value = std::nullopt);
  std::pair<std::string, std::string> parseInitialization(const std::string& initialization) const;
  BPMNOS::number evaluateExpression(const std::string& expression) const;
  BPMNOS::number getEffectiveDisclosure(size_t instanceId, const BPMN::Node* node, BPMNOS::number ownDisclosure);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_DynamicDataProvider_H
