#ifndef BPMNOS_Model_ExpectedValueFactory_H
#define BPMNOS_Model_ExpectedValueFactory_H

#include <limex.h>
#include <cmath>

namespace BPMNOS {

/**
 * @brief Factory for expected value functions in LIMEX expressions.
 *
 * Registers functions with the same names as random distributions,
 * but returns the expected (mean) value instead of sampling.
 *
 * Expected values:
 * - uniform(a, b): (a + b) / 2
 * - uniform_int(a, b): trunc((a + b) / 2)
 * - normal(mean, stddev): mean
 * - exponential(rate): 1 / rate
 * - poisson(mean): mean
 * - bernoulli(p): p
 * - binomial(n, p): n * p
 * - gamma(shape, scale): shape * scale
 * - lognormal(logscale, shape): exp(logscale + shape^2 / 2)
 * - geometric(p): (1 - p) / p
 */
class ExpectedValueFactory {
public:
  /**
   * @brief Register all expected value functions with the given LIMEX handle.
   */
  void registerFunctions(LIMEX::Handle<double>& handle);
};

} // namespace BPMNOS

#endif // BPMNOS_Model_ExpectedValueFactory_H
