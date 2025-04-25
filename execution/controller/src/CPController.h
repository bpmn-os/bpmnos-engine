#ifndef BPMNOS_Execution_CPController_H
#define BPMNOS_Execution_CPController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "FlattenedGraph.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "dispatcher/ReadyHandler.h"
#include "dispatcher/DeterministicTaskCompletion.h"
#include "model/bpmnos/src/extensionElements/Gatekeeper.h"
#include "model/bpmnos/src/extensionElements/Choice.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Content.h"
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
  void synchronizeSolution(const Token* token);
  void synchronizeStatus(const Token* token, const Vertex* vertex);
  void synchronizeData(const Token* token, const Vertex* vertex);
  void synchronizeGlobals(const Token* token, const Vertex* vertex);
//  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
  const CP::Model& getModel() const { return model; }
  const std::vector<const Vertex*>& getVertices() const { return vertices; }
protected:
  ReadyHandler readyHandler;
  DeterministicTaskCompletion completionHandler;
  Evaluator* evaluator;
  std::shared_ptr<TerminationEvent> terminationEvent;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
  const BPMNOS::Model::Scenario* scenario;
  Config config;
//  std::vector< const BPMNOS::Model::Scenario::InstanceData* > instances;
public:
  const FlattenedGraph flattenedGraph;
  CP::Model model;
protected:
  LIMEX::Handle<CP::Expression,CP::Expression> limexHandle;
  std::unordered_map< const Vertex*, const Vertex* > performing; /// Map holding the entry vertex of a sequential activity performed by a sequential performer

  void createCP(); /// Method creating the constraint program
  void createGlobalVariables();
  void createMessageFlowVariables();
  void createMessagingConstraints();
  void createMessageHeader(const Vertex* vertex);
  void createMessageContent(const Vertex* vertex);
  std::vector< const Vertex* > getReachableVertices(const Vertex* initialVertex); /// Returns a topologically sorted vector of all vertices reachable from the given vertex
  void initializeVertices(const Vertex* initialVertex);
  void createVertexVariables(const Vertex* vertex);
  void createEntryVariables(const Vertex* vertex);
  void createExitVariables(const Vertex* vertex);
  void createSequenceFlowVariables(const Vertex* source, const Vertex* target, const BPMNOS::Model::Gatekeeper* gatekeeper = nullptr);
  void createStatusFlowVariables(const Vertex* source, const Vertex* target);

  void createSequenceConstraints(const Vertex* vertex);
  void createRestrictions(const Vertex* vertex);
  CP::Expression createExpression(const Vertex* vertex, const Model::Expression& expression);

  void createGlobalIndexVariable(const Vertex* vertex);
  void createDataVariables(const Vertex* vertex);
  void createDataIndexVariables(const Vertex* vertex);

  void constrainGlobalVariables();
  void constrainDataVariables(const FlattenedGraph::Vertex* vertex);
  void constrainSequentialActivities();
  void constrainEventBasedGateway(const FlattenedGraph::Vertex* gateway);
  
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

  void addToObjective(const BPMNOS::Model::Attribute* attribute, const CP::Variable& variable);
  void addObjectiveCoefficients(const Vertex* vertex);

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
  void addAttributes(const Vertex* vertex, std::vector<AttributeVariables>& variables, const BPMNOS::Model::Attribute* loopIndex = nullptr);
  void createEntryStatus(const Vertex* vertex);
  void createExitStatus(const Vertex* vertex);
  void createLocalAttributeVariables(const Vertex* vertex);
  CP::Expression createOperatorExpression( const Model::Expression& operator_, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables );
  std::pair< CP::Expression, CP::Expression > getLocalAttributeVariables( const Model::Attribute* attribute, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables );

  const BPMNOS::Model::Choice* findChoice(const std::vector< std::unique_ptr<BPMNOS::Model::Choice> >& choices, const BPMNOS::Model::Attribute* attribute) const;

  const BPMNOS::Model::Content* findContent(const BPMNOS::Model::MessageDefinition* messageDefinition, const BPMNOS::Model::Attribute* attribute) const;
  
  std::vector<CPController::AttributeVariables> createUniquelyDeducedEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector<AttributeVariables>& inheritedStatus);
  std::vector<AttributeVariables> createAlternativeEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives);
  std::vector<AttributeVariables> createMergedStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs);
  CP::Expression getLoopIndex(const Vertex* vertex);
  void createLoopEntryStatus(const Vertex* vertex);
  std::vector<CPController::AttributeVariables> createLoopExitStatus(const Vertex* vertex);
    
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
  std::vector<const Vertex*> messageRecipients; /// Container of all (exit) vertices catching a message
  std::vector<const Vertex*> messageSenders; /// Container of all (entry) vertices throwing a message

  std::unordered_map< const Vertex*, const CP::Variable& > position; /// Variables holding sequence positions for all vertices
  std::unordered_map< const Vertex*, const CP::Variable& > visit; /// Variables indicating whether the a token enters or leaves a vertex

  std::unordered_map< std::pair< const Vertex*, const Vertex* >, const CP::Variable&, pair_hash > tokenFlow; /// Variables indicating whether the a token flows from one vertex to another
  std::unordered_map< std::pair< const Vertex*, const Vertex* >, const CP::Variable&, pair_hash > messageFlow; /// Variables indicating whether the a token flows from one vertex to another

  std::unordered_map< const Vertex*, std::unordered_map< std::string, AttributeVariables> > messageHeader; /// Variables representing the header of a message originating at a vertex
  std::unordered_map< const Vertex*, std::unordered_map< std::string, AttributeVariables> > messageContent; /// Variables representing the content of a message originating at a vertex

  std::vector< IndexedAttributeVariables > globals; /// Variables representing global attributes after i-th modification
  std::unordered_map< const Vertex*, const CP::Variable& > globalIndex; /// Variables representing an index representing the state of the global attributes

  std::unordered_map< std::pair< const Vertex*, const Model::Attribute*>, IndexedAttributeVariables, pair_hash > data; /// Variables representing data attributes after i-th modification
  std::unordered_map< const Vertex*, CP::reference_vector< const CP::Variable > > dataIndex; /// Variables representing an index representing the state of the data attributes for each data owner

  std::unordered_map< const Vertex*, std::vector<AttributeVariables> > status; /// Variables representing status attributes of a vertex
  std::unordered_map< const Vertex*, std::vector< std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> > > > locals; /// Variables representing status, data, globals attributes of a vertex after the i-th operator


  std::unordered_map< std::pair< const Vertex*, const Vertex* >, std::vector<AttributeVariables>, pair_hash > statusFlow; /// Variables representing status attributes flowing from one vertex to another

  std::list< const Vertex* > pendingVertices; /// The list of vertices to be processed
  std::list< const Vertex* > processedVertices; /// The list of vertices already processed
  bool hasPendingPredecessor(const Vertex* vertex);
  void unvisited(const Vertex* vertex);
  void finalizePredecessorPositions(const Vertex* vertex);
  std::list< const Vertex* >::iterator finalizeVertexPosition(const Vertex* vertex); /// Method finalizing the sequence position of a pending vertex and removing it from the list
  std::list< const Vertex* >::iterator finalizeUnvisitedTypedStartEvents(const Token* token, std::list< const Vertex* >::iterator it); /// Method finalizing the sequence position of a unvisited vertices belonging to typed start events
  size_t lastPosition;
public:
  virtual CP::Solution& createSolution(); /// Method creating a solution of the CP
  const CP::Solution& getSolution() const; /// Method providing access to the solution of the CP
  void setMessageDeliveryVariableValues( const Vertex* sender, const Vertex* recipient, BPMNOS::number timestamp );
protected:
  void initializeEventQueue(); /// Method creating an initial sequence of vertices

  const Vertex* getVertex( const Token* token ) const;
  std::optional< BPMN::Activity::LoopCharacteristics > getLoopCharacteristics(const Vertex* vertex) const;  
  std::optional< BPMNOS::number > getTimestamp( const Vertex* vertex ) const;
  void setTimestamp( const Vertex* vertex, BPMNOS::number timestamp );
  void setLocalStatusValue( const Vertex* vertex, size_t attributeIndex, BPMNOS::number value );
  std::pair< CP::Expression, CP::Expression > getAttributeVariables( const Vertex* vertex, const Model::Attribute* attribute);

  virtual std::shared_ptr<Event> createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex); /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex); /// Method creating a message delivery event from CP solution
  std::unique_ptr<CP::Solution> _solution;
public:
  const Vertex* entry(const Vertex* vertex);
  const Vertex* exit(const Vertex* vertex);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPController_H

