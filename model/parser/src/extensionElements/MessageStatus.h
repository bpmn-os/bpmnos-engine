#ifndef BPMNOS_Model_MessageStatus_H
#define BPMNOS_Model_MessageStatus_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Status.h"
#include "Message.h"

namespace BPMNOS::Model {

class MessageStatus : public Status{
public:
  MessageStatus(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  Message message;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageStatus_H
