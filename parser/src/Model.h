#ifndef BPMNOS_Model_H
#define BPMNOS_Model_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Status.h"
#include "Gatekeeper.h"
#include "JobShop.h"
#include "ResourceActivity.h"
#include "RequestActivity.h"
#include "ReleaseActivity.h"

namespace BPMNOS {

/**
 * @brief Represents a BPMN model with all its processes.
 *
 * The `Model` class encapsulates all processes with their nodes and sequence flows of a BPMN model.
 */
class Model : public BPMN::Model {
public:
  Model(const std::string& filename);

public:
  std::unique_ptr<BPMN::Process> createProcess(XML::bpmn::tProcess* process) override;
  std::unique_ptr<BPMN::FlowNode> createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::SequenceFlow> createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) override;
  std::unique_ptr<BPMN::FlowNode> createSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) override;
};

} // namespace BPMNOS

#endif // BPMNOS_Model_H
