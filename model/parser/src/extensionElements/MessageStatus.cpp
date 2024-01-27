#include "MessageStatus.h"

using namespace BPMNOS::Model;

MessageStatus::MessageStatus(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : Status(baseElement,parent)
  , message(Message(baseElement,parent))
{
}
