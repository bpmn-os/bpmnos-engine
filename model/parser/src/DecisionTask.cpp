#include "DecisionTask.h"

using namespace BPMNOS;

DecisionTask::DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent)
  : BPMN::Node(task)
  , BPMN::Task(task,parent)
{
  throw std::logic_error("DecisionTask: not yet implemented");
/*
  for ( auto attribute : choices ) {
    attribute->isImmutable = false;
  }
*/
}

void DecisionTask::apply(const Values& choices, Values& status) const {
  throw std::logic_error("DecisionTask: not yet implemented");
}

