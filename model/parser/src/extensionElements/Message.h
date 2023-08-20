#ifndef BPMNOS_Message_H
#define BPMNOS_Message_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"

namespace BPMNOS {

class Message : public BPMN::ExtensionElements {
public:
  Message(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::string name;
  std::optional< std::unique_ptr<Parameter> > request; ///< Optional reference to request associated to message.
  std::vector< std::unique_ptr<Content> > contents;
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
};

} // namespace BPMNOS

#endif // BPMNOS_Message_H
