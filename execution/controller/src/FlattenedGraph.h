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
  std::vector< Vertex > vertices; /// Map holding entry and exit vertices of each possible instantiation of a node
  std::unordered_map< const BPMN::Node*, std::unordered_map< BPMNOS::number, std::vector< std::reference_wrapper<Vertex> > > > vertexMap; /// Map holding entry and exit vertices of each possible instantiation of a node

  void addInstance( const BPMNOS::Model::Scenario::InstanceData* instance );
private:
  void addNonInterruptingEventSubProcess( const BPMN::EventSubProcess* eventSubProcess, Vertex& parentEntry, Vertex& parentExit );
  void addSender( const BPMN::MessageThrowEvent* messageThrowEvent, Vertex& senderEntry, Vertex& senderExit );
  void addRecipient( const BPMN::MessageCatchEvent* messageCatchEvent, Vertex& recipientEntry, Vertex& recipientExit );

  std::pair<Vertex&, Vertex&> createVertexPair(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node);
  void createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Activity* node);
//  void createChildren(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  void flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  
  std::vector< std::tuple<const BPMN::EventSubProcess*, Vertex&, Vertex&, unsigned int> > nonInterruptingEventSubProcesses;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > sendingVertices;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > receivingVertices;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_PrecedenceGraph_H
