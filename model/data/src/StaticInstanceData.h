#ifndef BPMNOS_StaticInstanceData_H
#define BPMNOS_StaticInstanceData_H

#include "InstanceData.h"

namespace BPMNOS {

class StaticDataProvider;

class StaticInstanceData : public InstanceData {
  friend class StaticDataProvider;
public:
  /**
   * @brief Constructor for StaticInstanceData.
   *
   * @param process The BPMN process associated with the instance data.
   * @param id The unique identifier of the instance.
   */
  StaticInstanceData(const BPMN::Process& process, const std::string& id);
  ~StaticInstanceData() = default;

  /**
   * @brief Virtual method returning a prediction of the attribute value and std::nullopt if no prediction can be made or the value is predicted to be undefined.
   */
  virtual Value getPredictedValue(const BPMNOS::Attribute* attribute) override;

  /**
   * @brief Virtual method returning an assumption on the attribute value and std::nullopt if no assumption can be made or the value is assumed to be undefined.
   */
  virtual Value getAssumedValue(const BPMNOS::Attribute* attribute) override;
};

} // namespace BPMNOS

#endif // BPMNOS_StaticInstanceData_H
