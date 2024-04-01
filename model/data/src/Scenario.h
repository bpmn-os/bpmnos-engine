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
 * @brief The Scenario class holds data for all BPMN instances.
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
    long unsigned int id; ///< Instance identifier.
    Data instantiation; ///< Data regarding the time of instantiation.
    std::unordered_map< const Attribute*, Data > data; ///< Data regarding attribute values.
  };


  /**
   * @brief Constructor for Scenario.
   */
  Scenario(const Model* model, BPMNOS::number inception, BPMNOS::number completion, const DataInput& attributes, unsigned int index = 0);
  /**
   * @brief Copy constructor for Scenario.
   */
  Scenario(const Scenario& other, unsigned int index);

  const Model& getModel() const;

  unsigned int index;

  static constexpr char delimiter = '^'; ///< Delimiter used for disambiguation of identifiers of non-interrupting event subprocesses

  /**
   * @brief Virtual method allowing derived scenarios to update their data.
   */
  virtual void update() {};

  /**
   * @brief Method returning the time of the earliest instantiation.
   */
  BPMNOS::number getInception() const;

  /**
   * @brief Method updating the completion time.
   */
  void updateCompletion(const BPMNOS::number time);

  /**
   * @brief Method returning true if the currentTime exceeds the completion time.
   */
  bool isCompleted(const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning a vector of all instances that are known to be instantiated at the given time.
   */
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning a vector of all instances that are anticipated to be instantiated at the assumed time.
   */
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getAnticipatedInstantiations(const BPMNOS::number currentTime, const BPMNOS::number assumedTime) const;

  /**
   * @brief Method returning a vector of all instances that have been created until the given time.
   */
  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning a vector of all instances that have been created or are known for sure until the given time.
   */
  std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning a vector of all instances that are anticipated and not known for sure at the given time.
   */
  std::vector< const InstanceData* > getAnticipatedInstances(const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning the initial status of a known instantiation at the given time.
   */
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const;

  /**
   * @brief Method returning the initial data attributes of a known instantiation at the given time.
   */
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const;

  /**
   * @brief Method returning the initial status of an anticipated instantiation at the given time.
   */
  BPMNOS::Values getAnticipatedInitialStatus(const InstanceData*, const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning the initial data attributes of an anticipated instantiation at the given time.
   */
  BPMNOS::Values getAnticipatedInitialData(const InstanceData*, const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the returns std::nullopt.
   */
  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the returns std::nullopt.
   */
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning the disclosed values of new attributes.
   */
  BPMNOS::Values getAnticipatedValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;

  /**
   * @brief Method returning the disclosed values of new attributes.
   */
  BPMNOS::Values getAnticipatedData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, Data instantiation);
  void removeAnticipatedInstance(const BPMNOS::number instanceId);

  Data& getInstantiationData(const BPMNOS::number instanceId);
  Data& getAttributeData(const BPMNOS::number instanceId, const Attribute* attribute);
  void addAnticipation( Data& data, Disclosure anticipation );
  void setRealization( Data& data, Disclosure realization );
protected:
  const Model* model;  ///< Pointer to the BPMN model.
  const DataInput& attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
  std::unordered_map<long unsigned int, InstanceData > instances; ///< Map of instances with key being the instance id.
  const Scenario::Disclosure& getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, const BPMNOS::number currentTime) const;
  BPMNOS::number inception; ///< Time earliest time in execution.
  BPMNOS::number completion; ///< The latest time in execution at which an instantiation can happen.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Scenario_H
