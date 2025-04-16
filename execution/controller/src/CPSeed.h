#ifndef BPMNOS_Execution_CPSeed_H
#define BPMNOS_Execution_CPSeed_H

#include <bpmn++.h>
#include "CPController.h"
#include <deque>
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
  static std::list<size_t> defaultSeed(std::list<size_t> partialSeed, size_t length);
  CP::Solution& createSolution() const; /// Returns a solution containing all sequence positions
  double coverage() const; /// Returns the ratio of the number vertices with sequence positions satisfying precedence constraints and the total number of vertices
  std::list<size_t> getSeed() const; /// Returns the seed corresponding to the final sequence
  std::vector<size_t> getSequence() const; /// Returns the sequence
private:
  CPController* controller;
  void initialize(std::list<size_t> seed);
  bool addSequencePosition(size_t index);
  std::vector<size_t> sequence;
  std::unordered_set<const FlattenedGraph::Vertex*> vertices;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPSeed_H

