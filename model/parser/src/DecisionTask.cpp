#include "DecisionTask.h"

using namespace BPMNOS;

DecisionTask::DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::Task(task,parent)
{
}
