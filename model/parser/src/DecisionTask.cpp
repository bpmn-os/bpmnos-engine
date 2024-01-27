#include "DecisionTask.h"

using namespace BPMNOS::Model;

DecisionTask::DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent)
  : BPMN::Node(task)
  , BPMN::FlowNode(task,parent)
  , BPMN::Task(task,parent)
{
}
