#ifndef BPMNOS_Model_RequestActivity_H
#define BPMNOS_Model_RequestActivity_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "xml/bpmnos/tAllocations.h"
#include "xml/bpmnos/tRequest.h"

namespace BPMNOS::Model {

class RequestActivity : public BPMN::SubProcess {
  friend class Model;
public:
  RequestActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  std::vector< std::reference_wrapper<XML::bpmnos::tRequest> > requests;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_RequestActivity_H
