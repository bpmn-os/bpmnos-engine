#ifndef BPMNOS_Model_SequentialAdHocSubProcess_H
#define BPMNOS_Model_SequentialAdHocSubProcess_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class SequentialAdHocSubProcess : public BPMN::AdHocSubProcess {
  friend class Model;
public:
  SequentialAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocsubProcess, BPMN::Scope* parent);
  BPMN::Node* performer;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SequentialAdHocSubProcess_H
