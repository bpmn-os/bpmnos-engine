#ifndef BPMNOS_Execution_SeededController_H
#define BPMNOS_Execution_SeededController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "execution/model/src/FlattenedGraph.h"
#include "execution/model/src/CPModel.h"
#include "execution/model/src/CPSolution.h"
#include "dispatcher/ReadyHandler.h"
#include "dispatcher/DeterministicTaskCompletion.h"
#include "model/bpmnos/src/extensionElements/Gatekeeper.h"
#include "model/bpmnos/src/extensionElements/Choice.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Content.h"
#include "model/utility/src/tuple_map.h"
#include <unordered_map>
#include <utility>
#include <memory>



namespace BPMNOS::Execution {

/**
 * @brief A controller dispatching decisions obtained from a solution of a constraint program
 */
class SeededController : public Controller {
public:
  using Vertex = FlattenedGraph::Vertex;
  using RequestType = Observable::Type;

  struct Config {
    bool instantEntry = false;
    bool instantExit = false;
  };
  static Config default_config() { return {}; } // Work around for compiler bug see: https://stackoverflow.com/questions/53408962/try-to-understand-compiler-error-message-default-member-initializer-required-be/75691051#75691051
  SeededController(const BPMNOS::Execution::FlattenedGraph* flattenedGraph, Config config = default_config());
  bool setSeed(const std::list<size_t> initialSeed);
  void connect(Mediator* mediator);
  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
  void synchronizeSolution(const Token* token);

protected:
  ReadyHandler readyHandler;
  DeterministicTaskCompletion completionHandler;
  Evaluator* evaluator;
  std::shared_ptr<TerminationEvent> terminationEvent;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
  Config config;
public:
  const FlattenedGraph* flattenedGraph;
protected:
  std::list<size_t> seed;
  std::unordered_map< const Vertex*, const Vertex* > performing; /// Map holding the entry vertex of a sequential activity performed by a sequential performer
    
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
  

  std::list< const Vertex* > pendingVertices; /// The list of vertices to be processed
  std::list< const Vertex* > processedVertices; /// The list of vertices already processed
  bool hasPendingPredecessor(const Vertex* vertex) const;
  bool hasPendingRecipient(const Vertex* vertex) const;
  void finalizePredecessorPositions(const Vertex* vertex);
  void fetchPendingPredecessors(std::unordered_set<const Vertex*>& predecessors, const Vertex* vertex) const;
  std::list< const Vertex* >::iterator finalizeVertexPosition(const Vertex* vertex); /// Method finalizing the sequence position of a pending vertex and removing it from the list
  std::list< const Vertex* >::iterator finalizeUnvisited(const Vertex* vertex);
  void finalizeUnvisitedChildren(const Vertex* vertex);
  std::list< const Vertex* >::iterator finalizeUnvisitedTypedStartEvents(std::list< const Vertex* >::iterator it); /// Method finalizing the sequence position of a unvisited vertices belonging to typed start events
public:
  std::list<size_t> getSequence() const; /// Method providing the vertex sequence in the solution
  void initializePendingVertices(); /// Method creating an initial sequence of vertices

  virtual std::shared_ptr<Event> createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) = 0; /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) = 0; /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) = 0; /// Method creating a choice event from CP solution
  virtual std::shared_ptr<Event> createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) = 0; /// Method creating a message delivery event from CP solution
  std::unique_ptr<CPSolution> _solution;
public:
  const Vertex* entry(const Vertex* vertex) const { return flattenedGraph->entry(vertex); };
  const Vertex* exit(const Vertex* vertex) const { return flattenedGraph->exit(vertex); };
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SeededController_H

