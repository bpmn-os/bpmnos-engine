#ifndef BPMNOS_Execution_PrecedenceGraph_H
#define BPMNOS_Execution_PrecedenceGraph_H

#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <bpmn++.h>
#include "model/data/src/Scenario.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents a precedence graph containing all BPMN nodes that may receive a token during execution of a scenario.
 *
 * For a given @ref BPMNOS::Model::Scenario "scenario", the `PrecedenceGraph` class encapsulates a precedence graph containing a vertex for each entry and each exit of a @ref BPMN::Node "node" in a BPMN model. The precedence graph includes all instances known at time zero.
 */
class PrecedenceGraph {
public:
  PrecedenceGraph(const BPMNOS::Model::Scenario* scenario);
  const BPMNOS::Model::Scenario* scenario;
  
  class Vertex {
  public:
    enum class Type { ENTRY, EXIT };
    Vertex(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node, Type type) : rootId(rootId), instanceId(instanceId), node(node), type(type) {};
    const BPMNOS::number rootId;
    const BPMNOS::number instanceId;
    const BPMN::Node* node;
    const Type type;
    std::vector<std::reference_wrapper<Vertex> > predecessors; /// Container holding all predecessors according to the token flow logic
    std::vector<std::reference_wrapper<Vertex> > successors;   /// Container holding all successors according to the token flow logic
//    std::vector<std::reference_wrapper<Vertex> > senders;      /// Container holding all possible sending vertices for a message catch event
//    std::vector<std::reference_wrapper<Vertex> > recipients;   /// Container holding all possible receiving vertices for a message throw event
  };

  std::vector< std::reference_wrapper<Vertex> > initialVertices; /// Container holding entry vertices of all process instances
  std::unordered_map< const BPMN::Node*, std::unordered_map< BPMNOS::number, std::vector< Vertex > > > vertices; /// Map holding entry and exit vertices of each possible instantiation of a node

private:
  std::vector< Vertex >& createVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node);
  std::vector< Vertex >& createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Activity* node);
//  void createChildren(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  void flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  std::list< std::tuple<BPMNOS::number, BPMNOS::number, const BPMN::EventSubProcess*> > nonInterruptingEventSubProcesses;
  std::unordered_map<const BPMN::FlowNode*,  std::list< std::pair<BPMNOS::number, Vertex&> > > sending;
  std::unordered_map<const BPMN::FlowNode*,  std::list< std::pair<BPMNOS::number, Vertex&> > > receiving;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_PrecedenceGraph_H
