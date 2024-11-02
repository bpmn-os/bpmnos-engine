#ifndef BPMNOS_Execution_FlattenedGraph_H
#define BPMNOS_Execution_FlattenedGraph_H

#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <bpmn++.h>
#include "model/data/src/Scenario.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents a graph containing all BPMN nodes that may receive a token during execution of a scenario.
 *
 * For a given @ref BPMNOS::Model::Scenario "scenario", the `FlattenedGraph` class encapsulates a graph containing a vertex for each entry and each exit of a @ref BPMN::Node "node" in a BPMN model. The flattened graph includes all instances known at time zero.
 */
class FlattenedGraph {
public:
  FlattenedGraph(const BPMNOS::Model::Scenario* scenario);
  const BPMNOS::Model::Scenario* scenario;
  
  class Vertex {
  public:
    enum class Type { ENTRY, EXIT };
    Vertex(size_t index, BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node, Type type);
    const size_t index;
    const BPMNOS::number rootId;
    const BPMNOS::number instanceId;
    const BPMN::Node* node;
    const Type type;
    std::vector< std::pair<const BPMN::SequenceFlow*, Vertex&> > inflows;      /// Container holding vertices connecting by an incoming sequence flow
    std::vector< std::pair<const BPMN::SequenceFlow*, Vertex&> > outflows;     /// Container holding vertices connecting by an outgoing sequence flow
    std::vector< std::reference_wrapper<Vertex> > predecessors; /// Container holding predecessors according to the execution logic (excl. sequence flows)
    std::vector< std::reference_wrapper<Vertex> > successors;   /// Container holding successors according to the execution logic (excl. sequence flows)
    std::vector< std::reference_wrapper<Vertex> > senders;      /// Container holding all possible vertices sending a message (or the message delivery notfication for a SendTask)
    std::vector< std::reference_wrapper<Vertex> > recipients;   /// Container holding all possible vertices receiving a message (or the message delivery notfication for a SendTask)
    std::vector< std::reference_wrapper<Vertex> > dataOwners;   /// Container holding all entry vertices of nodes owning at least one data attribute
    std::pair<Vertex&, Vertex&> parent(); /// Returns the vertices of the parent
    std::pair<Vertex&, Vertex&> performer(); /// Returns the vertices of the performer of a sequential activity vertex
    std::pair<Vertex&, Vertex&> dataOwner( const BPMNOS::Model::Attribute* attribute ); /// Returns the vertices of the owner of a data attribute
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
  void flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  
  std::vector< std::tuple<const BPMN::EventSubProcess*, Vertex&, Vertex&, unsigned int, Vertex*> > nonInterruptingEventSubProcesses;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > sendingVertices;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > receivingVertices;
  
  std::unordered_map<const Vertex*,  std::vector< std::pair<Vertex&, Vertex&> > > sequentialActivities; /// Container allowing to look up vertices of sequential activities given a pointer to the entry vertex of a performer  
  std::unordered_map<const Vertex*,  std::vector< std::pair<Vertex&, Vertex&> > > dataModifiers; /// Container allowing to look up vertices of tasks modifying data attributes given a pointer to the entry vertex of the node owning the data
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FlattenedGraph_H
