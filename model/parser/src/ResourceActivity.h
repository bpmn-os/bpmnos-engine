#ifndef BPMNOS_Model_ResourceActivity_H
#define BPMNOS_Model_ResourceActivity_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class ResourceActivity : public BPMN::SubProcess {
  friend class Model;
public:
  ResourceActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ResourceActivity_H
