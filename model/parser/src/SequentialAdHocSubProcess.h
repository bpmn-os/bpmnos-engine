#ifndef BPMNOS_Model_SequentialAdHocSubProcess_H
#define BPMNOS_Model_SequentialAdHocSubProcess_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

/**
 * @brief Class representing adhoc subprocesses with sequential ordering.
 *
 * Activities within an instance of the SequentialAdHocSubProcess class 
 * must not be executed in parallel. The sequential execution of such activities
 * is linked to a performer guaranteeing that at most one child activity of
 * any sequential adhoc subprocess linked to this performer is executed at any
 * point time.
 *
 * The performer relevant for a sequential adhoc subprocess can be specified
 * by adding a @ref XML::bpmn::tPerformer element with `name` being "Sequential" 
 * to the adhoc subprocess or an ancestor in the XML tree  (with no other sequential
 * adhoc subprocess between). If no such performer is provided, the adhoc subprocess
 * is assumed to have its own performer.
 */
class SequentialAdHocSubProcess : public BPMN::AdHocSubProcess {
  friend class Model;
public:
  SequentialAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocsubProcess, BPMN::Scope* parent);
  BPMN::Node* performer;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SequentialAdHocSubProcess_H
