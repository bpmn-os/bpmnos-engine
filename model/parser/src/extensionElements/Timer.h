#ifndef BPMNOS_Timer_H
#define BPMNOS_Timer_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "model/utility/src/Numeric.h"

namespace BPMNOS {

class Timer : public BPMN::ExtensionElements {
public:
  Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::unique_ptr<Parameter> trigger;

  template <typename T>
  T earliest(const std::vector<std::optional<T> >& values) const {
    if ( trigger->attribute.has_value() && values[trigger->attribute->get().index].has_value() ) {
      return values[trigger->attribute->get().index].value();
    }
    else if ( trigger->value.has_value() ) {
      return numeric<T>( std::stod( trigger->value->get().value ) );
    }
    return values[0].value();
  }  
};

} // namespace BPMNOS

#endif // BPMNOS_Timer_H
