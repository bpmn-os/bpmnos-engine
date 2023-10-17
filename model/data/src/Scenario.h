#ifndef BPMNOS_Scenario_H
#define BPMNOS_Scenario_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/Model.h"
#include "model/parser/src/extensionElements/Attribute.h"

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
    std::string id; ///< Instance identifier.
    Data instantiation; ///< Data regarding the time of instantiation.
    std::unordered_map< const Attribute*, Data > data; ///< Data regarding attribute values.
  };


  /**
   * @brief Constructor for Scenario.
   */
  Scenario(const Model* model, const DataInput& attributes, unsigned int index = 0);
  /**
   * @brief Copy constructor for Scenario.
   */
  Scenario(const Scenario& other, unsigned int index);

  const Model& getModel() const;

  unsigned int index;

  /**
   * @brief Virtual method allowing derived scenarios to update their data.
   */
  virtual void update() {};

  /**
   * @brief Method returning a vector of all instances that are known until the given time.
   */
  std::vector< const InstanceData* > getKnownInstances(BPMNOS::number time) const;

  /**
   * @brief Method returning a vector of all instances that are known to be instantiated at the given time.
   */
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > getKnownInstantiations(BPMNOS::number time) const;


  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the returns std::nullopt.
   */
  std::optional<BPMNOS::Values> getKnownValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time) const;


  /**
   * @brief Method returning a vector of all instances that have been disclosed until the given time.
   */
  std::vector< const InstanceData* > getAssumedInstances(BPMNOS::number currentTime, BPMNOS::number assumedTime) const;

  /**
   * @brief Method returning a vector of all instances that are assumed to be instantiated at the given time.
   */
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > getAssumedInstantiations(BPMNOS::number currentTime, BPMNOS::number assumedTime) const;

  /**
   * @brief Method returning the disclosed values of new attributes.
   */
  BPMNOS::Values getAssumedValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number currentTime, BPMNOS::number assumedTime) const;

  void addInstance(const BPMN::Process* process, const std::string& identifier, Data instantiation);
  void removeAnticipatedInstance(const std::string& identifier);

  Data& getInstantiationData(std::string instanceId);
  Data& getAttributeData(std::string instanceId, const Attribute* attribute);
  void addAnticipation( Data& data, Disclosure anticipation );
  void setRealization( Data& data, Disclosure realization );
protected:
  const Model* model;  ///< Pointer to the BPMN model.
  const DataInput& attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
  std::unordered_map<std::string, InstanceData > instances; ///< Map of instances with key being the instance id.
  const Scenario::Disclosure& getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, BPMNOS::number time) const;
  /**
   * @brief Method returning the initial status of a known instantiation at the given time.
   */
  Values getKnownInitialStatus(const InstanceData*, BPMNOS::number time) const;
  /**
   * @brief Method returning the initial status of a assumed instantiation at the given time.
   */
  BPMNOS::Values getAssumedInitialStatus(const InstanceData*, BPMNOS::number currentTime, BPMNOS::number assumedTime) const;

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Scenario_H
