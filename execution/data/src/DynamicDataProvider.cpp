#include "DynamicDataProvider.h"
#include <cmath>
#include <stdexcept>

using namespace BPMNOS::Execution;

DynamicDataProvider::DynamicDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION", "DISCLOSURE" }, sharedModel->limexHandle);
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

BPMNOS::number DynamicDataProvider::getKnownTime(size_t instanceId) const {
  return disclosureTimes.at(instanceId).at(instances.at(instanceId).process);
}

BPMNOS::number DynamicDataProvider::getProcessReadyTime(size_t instanceId) const {
  return std::max(StaticDataProvider::getProcessReadyTime(instanceId), getKnownTime(instanceId));
}

BPMNOS::number DynamicDataProvider::getActivityReadyTime(size_t instanceId, const BPMN::Node* activity) const {
  auto& nodeDisclosureTimes = disclosureTimes.at(instanceId);
  if ( auto it = nodeDisclosureTimes.find(activity); it != nodeDisclosureTimes.end() ) {
    return it->second;
  }
  return StaticDataProvider::getActivityReadyTime(instanceId, activity);
}
