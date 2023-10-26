#ifndef BPMNOS_Model_JobShop_H
#define BPMNOS_Model_JobShop_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class ResourceActivity;

class JobShop : public BPMN::SubProcess {
  friend class Model;
public:
  JobShop(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  ResourceActivity* resourceActivity;
protected:
  ResourceActivity* getResource();
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_JobShop_H
