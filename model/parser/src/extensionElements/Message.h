#ifndef BPMNOS_Message_H
#define BPMNOS_Message_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/utility/src/Numeric.h"

namespace BPMNOS {

class Message : public BPMN::ExtensionElements {
public:
  Message(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::string& name;
//  std::optional< std::unique_ptr<Parameter> > recipient;
//  std::optional< std::unique_ptr<Parameter> > sender;
  std::optional< std::unique_ptr<Parameter> > request;
  std::vector< std::unique_ptr<Content> > contents;
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
/*
  template <typename T>
  T receive(const std::vector<std::optional<T> >& values) const {
    if ( trigger->attribute.has_value() && values[trigger->attribute->get().index].has_value() ) {
      return values[trigger->attribute->get().index].value();
    }
    else if ( trigger->value.has_value() ) {
      return numeric<T>( std::stod( trigger->value->get().value ) );
    }
    return values[0].value();
  }  

  template <typename T>
  T send(const std::vector<std::optional<T> >& values) const {
    if ( trigger->attribute.has_value() && values[trigger->attribute->get().index].has_value() ) {
      return values[trigger->attribute->get().index].value();
    }
    else if ( trigger->value.has_value() ) {
      return numeric<T>( std::stod( trigger->value->get().value ) );
    }
    return values[0].value();
  }  

*/
};

} // namespace BPMNOS

#endif // BPMNOS_Message_H
