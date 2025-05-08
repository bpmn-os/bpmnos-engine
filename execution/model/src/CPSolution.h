#ifndef BPMNOS_Execution_CPSolution_H
#define BPMNOS_Execution_CPSolution_H

#include <bpmn++.h>
#include "FlattenedGraph.h"
#include "CPModel.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Observer.h"
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
 * @brief A solution of a constraint program
 */
class CPSolution : public Observer {
public:
  using Vertex = FlattenedGraph::Vertex;

  CPSolution(const CPModel& cp);
  void notice(const Observable* observable) override;
  void subscribe(Engine* engine);
  void unsubscribe(Engine* engine);
  void synchronize(const Token* token);
  void synchronize(const Event* event);
  const CPModel& cp;
  const FlattenedGraph& flattenedGraph;
  CP::Solution _solution;
  size_t lastPosition;
  const CP::Model& getModel() const { return cp.getModel(); }
  const CP::Solution& getSolution() const { return _solution; }
  
  void unvisitEntry(const Vertex* vertex);
  void unvisitExit(const Vertex* vertex);

/*
  void visit(const Vertex* vertex);
  
  void visitEntry(const Vertex* vertex, double timestamp); /// Method setting the position, visit, and timestamp variable of a vertex
  void visitExit(const Vertex* vertex, double timestamp); /// Method setting the position, visit, and timestamp variable of a vertex
*/
  void synchronizeStatus(const BPMNOS::Values& status, const Vertex* vertex);
  void synchronizeData(const BPMNOS::SharedValues& data, const Vertex* vertex);
  void synchronizeGlobals(const BPMNOS::Values& globals, const Vertex* vertex);

  std::vector<size_t> getSequence() const; /// Method providing the vertex sequence in the solution
  size_t getPosition(const Vertex* vertex) const; 
  void initializePositions(const std::vector<double>& positions); 
  void setPosition(const Vertex* vertex, size_t position);
  void finalizePosition(const Vertex* vertex);
  void finalizeUnvistedSubsequentPositions(const Vertex* vertex);

  bool isVisited(const Vertex* vertex) const;
  bool isUnvisited(const Vertex* vertex) const;
  
  void setTriggeredEvent( const Vertex* gateway, const Vertex* event );
  void setMessageDeliveryVariableValues( const Vertex* sender, const Vertex* recipient, BPMNOS::number timestamp );
  bool messageFlows( const Vertex* sender, const Vertex* recipient );

  std::optional< BPMN::Activity::LoopCharacteristics > getLoopCharacteristics(const Vertex* vertex) const;  
  std::optional< BPMNOS::number > getTimestamp( const Vertex* vertex ) const;
  void setTimestamp( const Vertex* vertex, BPMNOS::number timestamp );
  std::optional< BPMNOS::number > getStatusValue( const Vertex* vertex, size_t attributeIndex ) const;
  void setLocalStatusValue( const Vertex* vertex, size_t attributeIndex, BPMNOS::number value );
  std::pair< CP::Expression, CP::Expression > getAttributeVariables( const Vertex* vertex, const Model::Attribute* attribute);

public:
  const Vertex* entry(const Vertex* vertex) const { return flattenedGraph.entry(vertex); };
  const Vertex* exit(const Vertex* vertex) const { return flattenedGraph.exit(vertex); };
  bool complete() const { return _solution.complete(); };
  std::optional<double> getObjectiveValue() const { return _solution.getObjectiveValue(); };
  std::string errors() const { return _solution.errors(); };
  std::string stringify() const { return _solution.stringify(); };
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPSolution_H

