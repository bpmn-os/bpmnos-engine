#ifndef BPMNOS_Model_H
#define BPMNOS_Model_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>

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
/*
  std::unique_ptr<BPMN::SequenceFlow> createSequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, BPMN::Node* scope) override;
*/
};

} // namespace BPMNOS

#endif // BPMNOS_Model_H
