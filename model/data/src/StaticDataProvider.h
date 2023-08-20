#ifndef BPMNOS_StaticDataProvider_H
#define BPMNOS_StaticDataProvider_H

#include "DataProvider.h"
#include "StaticInstanceData.h"

namespace BPMNOS {

/**
 * @brief Class representing a data provider for static BPMN instance data.
 *
 * The StaticDataProvider class is responsible for providing and managing instance data
 * for BPMN processes. It reads instance data from a file and creates StaticInstanceData
 * objects for each instance. The created instances are stored in a map for
 * easy access and retrieval.
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
protected:
  void readInstances(csv::CSVReader& reader);
};

} // namespace BPMNOS

#endif // BPMNOS_StaticDataProvider_H
