namespace Test {

/// Method returning solution with sequence positions matching the topological sorting of vertices
CP::Solution& createDefaultSolution(Execution::CPController& controller) {
  auto& solution = controller.createSolution();
  auto& sequence = controller.getModel().getSequences().front();

  auto defaultSequenceValues = [](size_t n) {
    std::vector<double> values(n);
    std::iota(values.begin(), values.end(), 1.0);
    return values;
  };

  solution.setSequenceValues( sequence, defaultSequenceValues( sequence.variables.size() ) );
  return solution;
}

} // end namespace Test

#include "process/test.h"


