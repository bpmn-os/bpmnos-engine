#ifndef BPMNOS_Model_Sequencer_H
#define BPMNOS_Model_Sequencer_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class Sequencer : public BPMN::SubProcess {
  friend class Model;
public:
  Sequencer(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  Sequencer* reference;
  void addJob(BPMN::Activity* job);
  const std::vector<BPMN::Activity*>& getJobs();
protected:
  std::vector<BPMN::Activity*> jobs;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Sequencer_H
