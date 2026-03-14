#ifndef BPMNOS_Model_Scenario_H
#define BPMNOS_Model_Scenario_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Model {

typedef std::unordered_map< const BPMN::Process*, std::unordered_map< std::string, const Attribute* > > DataInput;

/**
 * @brief Abstract base class for scenarios holding data for all BPMN instances.
 */
class Scenario {
public:
  struct Disclosure {
    BPMNOS::number disclosure; ///< Time at which the value is disclosed.
    std::optional<BPMNOS::number> value; ///< Value that the attribute takes at the time of disclosed.
  };

  struct Data {
    std::vector<Disclosure> anticipations;
    std::optional<Disclosure> realization;
  };

  struct InstanceData {
    const BPMN::Process* process;
    size_t id; ///< Instance identifier.
    Data instantiation; ///< Data regarding the time of instantiation.
    std::unordered_map< const Attribute*, Data > data; ///< Data regarding attribute values.
  };

  virtual ~Scenario() = default;

  static constexpr char delimiters[] = {'^','#'}; ///< Delimiters used for disambiguation of identifiers of non-interrupting event subprocesses and multi-instance activities

  /**
   * @brief Virtual method allowing derived scenarios to update their data.
   */
  virtual void update() {};

  /**
   * @brief Method returning the model.
   */
  virtual const Model* getModel() const = 0;

  /**
   * @brief Method returning the time of the earliest instantiation.
   */
  virtual BPMNOS::number getInception() const = 0;

  /**
   * @brief Method returning true if the currentTime exceeds the completion time.
   */
  virtual bool isCompleted(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that are known to be instantiated at the given time.
   */
  virtual std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that are anticipated to be instantiated at the assumed time.
   */
  virtual std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getAnticipatedInstantiations(const BPMNOS::number currentTime, const BPMNOS::number assumedTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created until the given time.
   */
  virtual std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created or are known for sure until the given time.
   */
  virtual std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that are anticipated and not known for sure at the given time.
   */
  virtual std::vector< const InstanceData* > getAnticipatedInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a known value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a known value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning the initial status of a known instantiation at the given time.
   */
  virtual Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const = 0;

  /**
   * @brief Method returning the initial data attributes of a known instantiation at the given time.
   */
  virtual Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const = 0;

  /**
   * @brief Method returning disclosed value of an attribute.
   *
   * If no attribute value is yet disclosed, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getAnticipatedValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning disclosed value of an attribute.
   *
   * If no attribute value is yet disclosed, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getAnticipatedValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning the initial status of an anticipated instantiation at the given time.
   */
  virtual BPMNOS::Values getAnticipatedInitialStatus(const InstanceData*, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning the initial data attributes of an anticipated instantiation at the given time.
   */
  virtual BPMNOS::Values getAnticipatedInitialData(const InstanceData*, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning the disclosed values of new attributes.
   */
  virtual BPMNOS::Values getAnticipatedValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning the disclosed values of new attributes.
   */
  virtual BPMNOS::Values getAnticipatedData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  virtual void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, Data instantiation) = 0;
  virtual void removeAnticipatedInstance(const BPMNOS::number instanceId) = 0;

  virtual Data& getInstantiationData(const BPMNOS::number instanceId) = 0;
  virtual Data& getAttributeData(const BPMNOS::number instanceId, const Attribute* attribute) = 0;
  virtual void addAnticipation( Data& data, Disclosure anticipation ) = 0;
  virtual void setRealization( Data& data, Disclosure realization ) = 0;

  BPMNOS::Values globals;
  const Model* model;  ///< Pointer to the BPMN model.
  unsigned int index;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Scenario_H
