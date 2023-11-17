#ifndef BPMNOS_Model_Gatekeeper_H
#define BPMNOS_Model_Gatekeeper_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Restriction.h"

namespace BPMNOS::Model {


class Gatekeeper : public BPMN::ExtensionElements {
public:
  Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::vector< std::unique_ptr<Restriction> > restrictions;

  template <typename T>
  bool restrictionsSatisfied(const std::vector<std::optional<T> >& values) const {
    for ( auto& restriction : restrictions ) {
      if ( !restriction->isSatisfied(values) ) {
        return false; 
      }
    }
    return true; 
  }  
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Gatekeeper_H
