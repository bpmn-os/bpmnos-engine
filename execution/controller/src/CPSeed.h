#ifndef BPMNOS_Execution_CPSeed_H
#define BPMNOS_Execution_CPSeed_H

#include <bpmn++.h>
#include "CPController.h"
#include <list>
#include <unordered_set>

namespace BPMNOS::Execution {

/**
 * @brief Class representing a seed solution for the CPController allowing to derive some or all variable values.
 */
class CPSeed {
  struct MessageVertex;
  using Matching = std::unordered_map< MessageVertex*, MessageVertex* >;
public:
  CPSeed( CPController* controller, std::list<size_t> seed );
  static std::list<size_t> defaultSeed(size_t length);
  bool isFeasible() const; /// Returns true if a vertex sequence is found that is free of contradictions
  CP::Solution& createSolution() const; /// Returns a solution containing all sequence positions
  Matching findAlternatingPath(MessageVertex* sender, MessageVertex* recipient) const; /// Tries to determine an alternating path containing a specific sender-recipient match
  void updateMatching(Matching& update); /// Changes the current matching using the update
private:
  CPController* controller;
  void initialize(std::list<size_t> seed);
  bool addSequencePosition(size_t index);
  std::vector<double> sequence;
  std::unordered_set<const FlattenedGraph::Vertex*> vertices;
  
  struct MessageVertex {
    MessageVertex(const FlattenedGraph::Vertex* vertex) : vertex(vertex) {};
    MessageVertex(const FlattenedGraph::Vertex* vertex, std::vector<MessageVertex>& senders);
    const FlattenedGraph::Vertex* vertex;
    std::vector<MessageVertex*> candidates;
  };

  std::vector<MessageVertex> senders;
  std::vector<MessageVertex> recipients;
  Matching matching; /// Map associating the matched recipient to each sender
  Matching findAugmentingPath(MessageVertex* recipient) const; /// Tries to determine an augmenting path for a new recipient
private:
  Matching findAlternatingPath(MessageVertex* recipient, std::unordered_set<MessageVertex*>& marked) const;
  Matching updateMatch(MessageVertex* sender, MessageVertex* recipient, std::unordered_set<MessageVertex*>& marked) const;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPSeed_H

