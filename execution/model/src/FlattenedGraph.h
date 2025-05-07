#ifndef BPMNOS_Execution_FlattenedGraph_H
#define BPMNOS_Execution_FlattenedGraph_H

#include <unordered_map>
#include <deque>
#include <vector>
#include <list>
#include <string>
#include <unordered_set>
#include <bpmn++.h>
#include <nlohmann/json.hpp>
#include "model/data/src/Scenario.h"
#include "model/utility/src/tuple_map.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Represents a graph containing all BPMN nodes that may receive a token during execution of a scenario.
 *
 * For a given @ref BPMNOS::Model::Scenario "scenario", the `FlattenedGraph` class encapsulates a graph containing a vertex for each entry and each exit of a @ref BPMN::Node "node" in a BPMN model. The flattened graph includes all instances known at time zero.
 */
class FlattenedGraph {
public:
  FlattenedGraph(const BPMNOS::Model::Scenario* scenario);
  const BPMNOS::Model::Scenario& scenario;
  nlohmann::ordered_json jsonify() const;
  
  class Vertex {
  public:
    enum class Type { ENTRY, EXIT };
    Vertex(FlattenedGraph* graph, BPMNOS::number rootId, BPMNOS::number instanceId, std::vector< size_t > loopIndices, const BPMN::Node* node, Type type, std::optional< std::pair<Vertex&, Vertex&> > parent);
    // Delete other constructors and assignment operators
    Vertex() = delete; // Prevents default construction
    Vertex(const Vertex&) = delete;           // Non-copyable
    Vertex& operator=(const Vertex&) = delete; // Non-copy-assignable
    Vertex(Vertex&&) = delete;               // Non-movable
    Vertex& operator=(Vertex&&) = delete;    // Non-move-assignable
    FlattenedGraph* graph;
    const size_t index; // Zero-based index allowing to access the vertex
    const BPMNOS::number rootId;
    const BPMNOS::number instanceId;
    const std::vector< size_t > loopIndices; // Container holding all loop indices (if any)
    const BPMN::Node* node;
    const Type type;
    std::optional< std::pair<Vertex&, Vertex&> > parent;      /// Parent vertices
    std::vector< std::pair<const BPMN::SequenceFlow*, Vertex&> > inflows;      /// Container holding vertices connecting by an incoming sequence flow, for loop activities the sequence flow may be a nullptr
    std::vector< std::pair<const BPMN::SequenceFlow*, Vertex&> > outflows;     /// Container holding vertices connecting by an outgoing sequence flow, for loop activities the sequence flow may be a nullptr
    std::vector< std::reference_wrapper<Vertex> > predecessors; /// Container holding predecessors according to the execution logic (excl. sequence flows)
    std::vector< std::reference_wrapper<Vertex> > successors;   /// Container holding successors according to the execution logic (excl. sequence flows)
    std::vector< std::reference_wrapper<Vertex> > senders;      /// Container holding all possible vertices sending a message (or the message delivery notfication for a SendTask)
    std::vector< std::reference_wrapper<Vertex> > recipients;   /// Container holding all possible vertices receiving a message (or the message delivery notfication for a SendTask)
    std::vector< std::reference_wrapper<Vertex> > dataOwners;   /// Container holding all entry vertices of nodes owning at least one data attribute
//    std::pair<const Vertex&, const Vertex&> parent() const; /// Returns the vertices of the parent
    const Vertex* performer() const ; /// Returns the entry vertex of the performer of a sequential activity vertex
    size_t dataOwnerIndex( const BPMNOS::Model::Attribute* attribute ) const; /// Returns the index of the data owner of an attribute
    std::pair<const Vertex&, const Vertex&> dataOwner( const BPMNOS::Model::Attribute* attribute ) const; /// Returns the vertices of the owner of a data attribute
    std::string reference() const; /// Returns a unique reference of the vertex
    std::string shortReference() const; /// Returns a reference of the vertex excluding the type
    nlohmann::ordered_json jsonify() const;
    template<typename T>
    bool entry() const { return (type == Type::ENTRY) && node->represents<T>(); }
    template<typename T>
    bool exit() const { return (type == Type::EXIT) && node->represents<T>(); }
  };

  std::vector< std::reference_wrapper<Vertex> > initialVertices; /// Container holding entry vertices of all process instances
  std::vector< const Vertex* > getSortedVertices() const; /// Returns a topologically sorted vector of all vertices reachable from the given vertex
  std::deque< Vertex > vertices; /// Container holding entry and exit vertices of each possible instantiation of a node
  std::unordered_set< const Vertex* > dummies; /// Container holding dummy vertices for loop & multi-instance activities
  
  const BPMNOS::Model::Attribute* getLoopIndexAttribute(const BPMN::Activity* activity) const; 
  std::unordered_map< const BPMN::Node*, std::vector< const BPMNOS::Model::Attribute* > > loopIndexAttributes; // Container holding all attributes representing loop indices for a given node
  tuple_map< std::tuple<BPMNOS::number, std::vector< size_t >, const BPMN::Node* >, std::pair<Vertex&, Vertex&> > vertexMap; /// Map holding entry and exit vertices of each possible instantiation of a node

  void addInstance( const BPMNOS::Model::Scenario::InstanceData* instance );
private:
  void addNonInterruptingEventSubProcess( const BPMN::EventSubProcess* eventSubProcess, Vertex& parentEntry, Vertex& parentExit );
  void addSender( const BPMN::MessageThrowEvent* messageThrowEvent, Vertex& senderEntry, Vertex& senderExit );
  void addRecipient( const BPMN::MessageCatchEvent* messageCatchEvent, Vertex& recipientEntry, Vertex& recipientExit );

  std::pair<Vertex&, Vertex&> createVertexPair(BPMNOS::number rootId, BPMNOS::number instanceId, std::vector< size_t > loopIndices, const BPMN::Node* node, std::optional< std::pair<Vertex&, Vertex&> > parent);
  void createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, std::vector< size_t > loopIndices, const BPMN::Activity* node, std::optional< std::pair<Vertex&, Vertex&> > parent);
  void flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit);
  
  std::vector< std::tuple<const BPMN::EventSubProcess*, Vertex&, Vertex&, unsigned int, Vertex*> > nonInterruptingEventSubProcesses;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > sendingVertices;
  std::unordered_map<const BPMN::FlowNode*,  std::vector< std::pair<Vertex&, Vertex&> > > receivingVertices;

public:  
  std::unordered_map<const Vertex*,  std::vector< std::pair<const Vertex&, const Vertex&> > > sequentialActivities; /// Container allowing to look up vertices of sequential activities given a pointer to the entry vertex of a performer  
  std::unordered_map<const Vertex*,  std::vector< std::pair<const Vertex&, const Vertex&> > > dataModifiers; /// Container allowing to look up vertices of tasks modifying data attributes given a pointer to the entry vertex of the node owning the data
  std::vector< std::pair<const Vertex&, const Vertex&> > globalModifiers; /// Container holding vertices of tasks modifying global attributes
  bool modifiesData(const Vertex& vertex, const Vertex& dataOwner) const; /// Method returning true of the vertex modifies a data attribute of the given owner 
  bool modifiesGlobals(const Vertex& vertex) const; /// Method returning true of the vertex modifies a global attribute

  const Vertex* getVertex( const Token* token ) const;

  const Vertex* entry(const Vertex* vertex) const;
  const Vertex* exit(const Vertex* vertex) const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FlattenedGraph_H
