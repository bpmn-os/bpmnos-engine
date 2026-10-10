#include "DynamicDataProvider.h"
#include <cmath>
#include <stdexcept>

using namespace BPMNOS::Execution;

DynamicDataProvider::DynamicDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, std::chrono::milliseconds clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION", "DISCLOSURE" }, this->model->limexHandle);
}

DynamicDataProvider::DynamicDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
}

void DynamicDataProvider::readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle) {
  enum { DISCLOSURE };
  auto& disclosure = row.cells[DISCLOSURE];
  auto& nodeDisclosureTimes = disclosureTimes[row.instanceId];

  // the disclosure time of a node is at least that of the scope containing it
  BPMNOS::number disclosureTime = 0;
  if ( auto childNode = row.node->represents<BPMN::ChildNode>() ) {
    auto scope = nodeDisclosureTimes.find(childNode->parent);
    if ( scope == nodeDisclosureTimes.end() ) {
      throw std::runtime_error("DynamicDataProvider: disclosure of '" + row.node->id + "' given before that of '" + childNode->parent->id + "'");
    }
    disclosureTime = scope->second;
  }

  if ( row.initialization.empty() ) {
    if ( !disclosure.empty() ) {
      throw std::runtime_error("DynamicDataProvider: disclosure of '" + row.node->id + "' requires an initialization in the same row");
    }
  }
  else {
    StaticDataProvider::readValue(reader, row, handle);
    if ( !disclosure.empty() ) {
      // the disclosure may refer to the value just initialized
      auto ownDisclosureTime = BPMNOS::number(std::ceil((double)reader.evaluate(row.instanceId, row.node, disclosure, DECIMAL, handle)));
      disclosureTime = std::max(disclosureTime, ownDisclosureTime);
    }
  }

  auto [it, inserted] = nodeDisclosureTimes.try_emplace(row.node, disclosureTime);
  if ( !inserted ) {
    it->second = std::max(it->second, disclosureTime);
  }
}

const std::unordered_map<const BPMN::Node*, BPMNOS::number>& DynamicDataProvider::getDisclosureTimes([[maybe_unused]] const Scenario& scenario, size_t instanceId) const {
  return disclosureTimes.at(instanceId);
}

BPMNOS::number DynamicDataProvider::getKnownTime(const Scenario& scenario, size_t instanceId) const {
  return getDisclosureTimes(scenario, instanceId).at(getInstance(instanceId).process);
}

BPMNOS::number DynamicDataProvider::getProcessReadyTime(const Scenario& scenario, size_t instanceId) const {
  return std::max(StaticDataProvider::getProcessReadyTime(scenario, instanceId), getKnownTime(scenario, instanceId));
}

BPMNOS::number DynamicDataProvider::getActivityReadyTime(const Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Status& readyStatus) const {
  auto& nodeDisclosureTimes = getDisclosureTimes(scenario, instanceId);
  if ( auto it = nodeDisclosureTimes.find(activity); it != nodeDisclosureTimes.end() ) {
    return std::max(it->second, StaticDataProvider::getActivityReadyTime(scenario, instanceId, activity, readyStatus));
  }
  return StaticDataProvider::getActivityReadyTime(scenario, instanceId, activity, readyStatus);
}
