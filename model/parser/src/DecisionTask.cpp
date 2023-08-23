#include "DecisionTask.h"

using namespace BPMNOS::Model;

DecisionTask::DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent)
  : BPMN::Node(task)
  , BPMN::Task(task,parent)
{
}
