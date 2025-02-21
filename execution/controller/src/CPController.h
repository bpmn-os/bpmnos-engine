#ifndef BPMNOS_Execution_CPController_H
#define BPMNOS_Execution_CPController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "FlattenedGraph.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Observer.h"
#include "model/utility/src/tuple_map.h"
#include <cp.h>
#include <unordered_map>
#include <utility>
#include <memory>



namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching decisions obtained from a solution of a constraint program
 */
class CPController : public Controller {
public:
  using Vertex = FlattenedGraph::Vertex;
  using RequestType = Observable::Type;

  struct Config {
    bool instantEntry = false;
    bool instantExit = false;
  };
  static Config default_config() { return {}; } // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  CPController(const BPMNOS::Model::Scenario* scenario, Config config = default_config());
  void connect(Mediator* mediator);
  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
  void validate(const Token* token);
//  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
  const CP::Model& getModel() const { return model; }
  const std::vector<const Vertex*> getVertices() const { return vertices; }
protected:
  Evaluator* evaluator;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
  const BPMNOS::Model::Scenario* scenario;
  Config config;
//  std::vector< const BPMNOS::Model::Scenario::InstanceData* > instances;
  const FlattenedGraph flattenedGraph;
  CP::Model model;
public:
protected:
  void createCP(); /// Method creating the constraint program
  void createGlobalVariables();
  void createMessageVariables();
  void createMessageContent(const Vertex* vertex);
  std::vector< const Vertex* > getReachableVertices(const Vertex* initialVertex); /// Returns a topologically sorted vector of all vertices reachable from the given vertex
  void initializeVertices(const Vertex* initialVertex);
  void createVertexVariables(const Vertex* vertex);
  void createEntryVariables(const Vertex* vertex);
  void createExitVariables(const Vertex* vertex);

  void createSequenceConstraints(const Vertex* vertex);
  void createRestrictions(const Vertex* vertex);
  CP::Expression createExpression(const Vertex* vertex, const Model::Expression& expression);

  void createGlobalIndexVariable(const Vertex* vertex);
  void createDataVariables(const Vertex* vertex);
  void createDataIndexVariables(const Vertex* vertex);
  
  struct AttributeVariables {
    const CP::Variable& defined;
    const CP::Variable& value;
    // Constructor
    AttributeVariables(const CP::Variable& defined, const CP::Variable& value)
        : defined(defined), value(value) {}
    // Copy Constructor
    AttributeVariables(const AttributeVariables& other)
        : defined(other.defined), value(other.value) {}
    // Assignment Operator
    AttributeVariables operator=(const AttributeVariables& other) {
        return AttributeVariables(other.defined,other.value);
    }
  };

  struct IndexedAttributeVariables {
    CP::IndexedVariables& defined;
    CP::IndexedVariables& value;
  };

  struct AttributeEvaluation {
    AttributeEvaluation(std::expected< double, std::string > defined, std::expected< double, std::string > value) : _defined(defined), _value(value) {}
    inline bool defined() const { return (bool)_defined.value(); }
    inline double value() const { return _value.value(); }
    // bool cast operator
    explicit operator bool() const {
      return static_cast<bool>(_defined) && static_cast<bool>(_value);
    }
  private:
    std::expected< double, std::string > _defined;
    std::expected< double, std::string > _value;
  };
  
  void createStatus(const Vertex* vertex);
  void createEntryStatus(const Vertex* vertex);
  void createExitStatus(const Vertex* vertex);
  std::vector<AttributeVariables> createAlternativeEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives);
  std::vector<AttributeVariables> createMergedStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs);
  
/*
  std::unordered_map<const BPMN::MessageThrowEvent*, std::vector<BPMNOS::number> > originInstances; /// Map containing all instance identifiers for all message throw events

  auto getReachableEndNodes( const BPMN::Scope* scope ) {
    // Check if the given scope exists in the map
    auto it = reachableFlowNodes.find(scope);
    if (it == reachableFlowNodes.end()) {
      assert(!"Unknown scope");
    }

    // Use ranges and views to filter nodes with empty outgoing vectors
    auto reachableEndNodes = it->second | std::ranges::views::filter([](const BPMN::FlowNode* node) {
        return node->outgoing.empty();
    });

    return reachableEndNodes;
  };
*/
  
  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        // return h1 ^ h2;
        // return h1 * 31 + h2; // Prime multiplication
        return h1 ^ (h2 * 2654435761u); // Uses a large prime for better distribution
    }
  };
  
  std::vector<const Vertex*> vertices; /// Container of all vertices considered
  BPMNOS::tuple_map< std::tuple< const BPMN::Node*, const BPMNOS::number, const Vertex::Type>, const Vertex* > vertexMap; /// Map holding allowing to access vertices by their node, instanceId, and type
  std::vector<const Vertex*> messageRecipients; /// Container of all vertices catching a message

  std::unordered_map< const Vertex*, const CP::Variable& > position; /// Variables holding sequence positions for all vertices
  std::unordered_map< const Vertex*, const CP::Variable& > visit; /// Variables indicating whether the a token enters or leaves a vertex

  std::unordered_map< std::pair< const Vertex*, const Vertex* >, const CP::Variable&, pair_hash > tokenFlow; /// Variables indicating whether the a token flows from one vertex to another
  std::unordered_map< std::pair< const Vertex*, const Vertex* >, const CP::Variable&, pair_hash > messageFlow; /// Variables indicating whether the a token flows from one vertex to another

  std::vector< IndexedAttributeVariables > globals; /// Variables representing global attributes after i-th modification
  std::unordered_map< const Vertex*, const CP::Variable& > globalIndex; /// Variables representing an index representing the state of the global attributes

  std::unordered_map< const Model::Attribute*, IndexedAttributeVariables > data; /// Variables representing data attributes after i-th modification
  std::unordered_map< const Vertex*, CP::reference_vector< const CP::Variable > > dataIndex; /// Variables representing an index representing the state of the data attributes for each data owner

  std::unordered_map< const Vertex*, std::vector<AttributeVariables> > status; /// Variables representing status attributes of a vertex
  std::unordered_map< std::pair< const Vertex*, const Vertex* >, std::vector<AttributeVariables>, pair_hash > statusFlow; /// Variables representing status attributes flowing from one vertex to another

  std::unordered_map< const Vertex*, std::vector< std::tuple< std::string_view, size_t, AttributeVariables> > > messageContent; /// Variables representing status attributes of a vertex


  std::queue< std::pair< RequestType, const Vertex* > > decisionQueue; /// The queue of decisions to be made
public:
  virtual CP::Solution& createSolution(); /// Method creating a solution of the CP
  const CP::Solution& getSolution() const; /// Method providing access to the solution of the CP
  void setMessageFlowVariableValue( const Vertex* sender, const Vertex* recipient );
protected:
  void createDecisionQueue(); /// Method creating the decision queue from CP
  std::optional< BPMNOS::number > getTimestamp( const Vertex* vertex ) const;
  void setTimestamp( const Vertex* vertex, BPMNOS::number timestamp );
  std::pair< CP::Expression, CP::Expression > getAttributeVariables( const Vertex* vertex, const Model::Attribute* attribute);

  virtual std::shared_ptr<Event> createEntryEvent(const SystemState* systemState, Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createExitEvent(const SystemState* systemState, Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createChoiceEvent(const SystemState* systemState, Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createMessageDeliveryEvent(const SystemState* systemState, Token* token, const Vertex* vertex); /// Method creating a message delivery event from CP solution

  const Vertex* entry(const Vertex* vertex);
  const Vertex* exit(const Vertex* vertex);
  std::unique_ptr<CP::Solution> _solution;
/* 
  std::unordered_map<const BPMNOS::Model::Attribute*, const BPMN::Scope* > dataOwner;/// Map allowing to look up the scope owning a data attribute
  std::unordered_map<const BPMN::Scope*, std::vector<const BPMN::Node* > > sequentialActivities;/// Map allowing to look up the sequential activities that may change a data attribute (assumimng that intermediate changes are not propagated)
  
  using NodeReference = std::pair<size_t, const BPMN::Node*>;
  using DataReference = std::pair<size_t, const BPMNOS::Model::Attribute*>;
  using MessageCatchReference = std::pair<size_t, const BPMN::MessageCatchEvent*>;
  using MessageThrowReference = std::pair<size_t, const BPMN::MessageThrowEvent*>;
  using ScopeReference = std::pair<size_t, const BPMN::Scope*>;
  using SequenceFlowReference = std::pair<size_t,const BPMN::SequenceFlow*>;

  template <typename T>
  std::string identifier(const std::pair<size_t, const T*>& reference) {
    return BPMNOS::to_string(reference.first,STRING) + "," + reference.second->id;
  }

  template <typename ReferenceType, typename ValueType>
  using variable_map = std::unordered_map<ReferenceType, ValueType, pair_hash>;


  variable_map<NodeReference, const CP::Variable&> nodeVariables; 
  variable_map<SequenceFlowReference, const CP::Variable&> sequenceFlowVariables; 
  variable_map<MessageCatchReference, variable_map<MessageThrowReference, const CP::Variable&> > messageFlowVariables; 
  
  std::vector<AttributeVariables> globals; 
  variable_map<DataReference, std::vector< AttributeVariables> > dataState; /// Variables representing data attributes after i-th activity of a sequential performer is conducted

  variable_map<NodeReference, variable_map<ScopeReference, std::vector< std::reference_wrapper< const CP::Variable > > > > entryDataState; /// Binary variables indicating the data states upon entry
  variable_map<NodeReference, variable_map<ScopeReference, std::vector< std::reference_wrapper< const CP::Variable > > > > exitDataState; /// Binary variables indicating the data states upon exit

  variable_map<NodeReference, std::vector<AttributeVariables> > entryData; /// Variables representing data state upon entry to a node
  variable_map<NodeReference, std::vector<AttributeVariables> > exitData;  /// Variables representing data state upon exit of a node

  variable_map<NodeReference, std::vector<AttributeVariables> > entryStatus;
  variable_map<NodeReference, std::vector<AttributeVariables> > exitStatus; 
  
  variable_map<NodeReference, std::vector< std::vector<AttributeVariables> > > interimData;  /// Variables representing data attributes after the i-th operator is applied
  variable_map<NodeReference, std::vector< std::vector<AttributeVariables> > > interimStatus;  /// Variables representing data attributes after i-th operator is applied


private:
  std::unordered_map<NodeReference, std::set<const BPMN::Scope*>, pair_hash > scopesOwningData;
  std::set<NodeReference> pendingEntry;
  std::list<NodeReference> pendingExit;
  void addChildren(ScopeReference reference);
  void addSuccessors(NodeReference reference);

  void addGlobalVariable( const BPMNOS::Model::Attribute* attribute, std::optional<BPMNOS::number> value );
  
  bool checkEntryDependencies(NodeReference reference);
  bool checkExitDependencies(NodeReference reference);
  
  void addEntryVariables(NodeReference reference);  
  void createNodeTraversalVariables(NodeReference reference); /// x_n
  void createEntryDataStateVariables(NodeReference reference); /// y^entry_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the entry data state at n for data belonging to s 
  void deduceEntryAttributeVariable(std::vector<AttributeVariables>& attributeVariables, const BPMNOS::Model::Attribute* attribute, NodeReference reference, variable_map<NodeReference, std::vector<AttributeVariables> >& entryAttributes, variable_map<NodeReference, std::vector<AttributeVariables> >& exitAttributes);

  void addExitVariables(NodeReference reference);
  void createExitDataStateVariables(NodeReference reference); /// y^exit_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the exit data state at n for data belonging to s 
  void constrainExitDataStateVariables(ScopeReference reference); /// y^exit_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the exit data state at n for data belonging to s 
//  void createSequenceFlowTraversalVariables(SequenceFlowReference reference);
*/
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPController_H

