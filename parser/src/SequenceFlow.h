#ifndef BPMNOS_SequenceFlow_H
#define BPMNOS_SequenceFlow_H

#include <bpmn++.h>
#include "xml/bpmnos/tRestrictions.h"
#include "Node.h"

namespace BPMNOS {

class Node;

/**
 * @brief Represents a sequence flow in a BPMN process.
 *
 * The `SequenceFlow` class encapsulates the information and relationships associated with a sequence flow
 * in a BPMN process. The class provides access to the underlying `tSequenceFlow` element and the source 
 * and target node.
 */
class SequenceFlow : public BPMN::SequenceFlow {
public:
  SequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, BPMN::Node* scope);
  std::vector< std::reference_wrapper<XML::bpmnos::tRestriction> > gatekeepers;
protected:
  template<class C, class T> std::vector< std::reference_wrapper<T> > getExtensionElements() {
    if ( sequenceFlow->extensionElements ) {
      if ( auto container = sequenceFlow->extensionElements->get().template getOptionalChild<C>() ) {
        return container->get().template getChildren<T>();
      }
    }
    return std::vector< std::reference_wrapper<T> >(); 
  };
};

} // namespace BPMNOS

#endif // BPMNOS_SequenceFlow_H

