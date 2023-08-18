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
  std::unique_ptr<BPMN::EventSubProcess> createEventSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::SequenceFlow> createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) override;
  std::unique_ptr<BPMN::FlowNode> createSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createTimerBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createTimerCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) override;

  void createMessageFlows() override;


};

} // namespace BPMNOS

#endif // BPMNOS_Model_H
