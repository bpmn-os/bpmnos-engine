#ifndef BPMNOS_Model_Sequencer_H
#define BPMNOS_Model_Sequencer_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class ResourceActivity;

class Sequencer : public BPMN::SubProcess {
  friend class Model;
public:
  Sequencer(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  ResourceActivity* resourceActivity;
protected:
  ResourceActivity* initializeResource();
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Sequencer_H
