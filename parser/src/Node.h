#ifndef BPMNOS_Node_H
#define BPMNOS_Node_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "SequenceFlow.h"
#include "xml/bpmnos/tStatus.h"
#include "xml/bpmnos/tRestrictions.h"
#include "xml/bpmnos/tOperators.h"

namespace BPMNOS {

class SequenceFlow;
class Model;

/**
 * @brief Represents a node in a BPMN process.
 *
 * The `Node` class encapsulates the information and relationships associated with a node in a BPMN process.
 * It can represent both a BPMN process itself or a flow node within a process. The class provides
 * functionalities to access the underlying XML element, the parent node, child nodes, start
 * nodes, and sequence flows associated with the current node.
 */
class Node : public BPMN::Node {
  friend class Model;
public:
  Node(XML::bpmn::tProcess& process);
  Node(XML::bpmn::tFlowNode& flowNode, BPMN::Node* parentNode);
  std::vector< std::reference_wrapper<XML::bpmnos::tAttribute> > status;
  std::vector< std::reference_wrapper<XML::bpmnos::tRestriction> > restrictions;
  std::vector< std::reference_wrapper<XML::bpmnos::tOperator> > operators;
protected:
  template<class C, class T> std::vector< std::reference_wrapper<T> > getExtensionElements() {
    if ( element->extensionElements ) {
      if ( auto container = element->extensionElements->get().template getOptionalChild<C>() ) {
        return container->get().template getChildren<T>();
      }
    }
    return std::vector< std::reference_wrapper<T> >(); 
  };
};

} // namespace BPMNOS

#endif // BPMNOS_Node_H
