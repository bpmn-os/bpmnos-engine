#ifndef BPMNOS_StaticInstanceData_H
#define BPMNOS_StaticInstanceData_H

#include "InstanceData.h"

namespace BPMNOS::Model {

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
  StaticInstanceData(const BPMN::Process* process, const std::string& id);
  ~StaticInstanceData() = default;

  /**
   * @brief Virtual method returning a prediction of the attribute value and std::nullopt if no prediction can be made or the value is predicted to be undefined.
   */
  virtual std::optional<BPMNOS::number> getPredictedValue(const Attribute* attribute) const override;

  /**
   * @brief Virtual method returning an assumption on the attribute value and std::nullopt if no assumption can be made or the value is assumed to be undefined.
   */
  virtual std::optional<BPMNOS::number> getAssumedValue(const Attribute* attribute) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_StaticInstanceData_H
