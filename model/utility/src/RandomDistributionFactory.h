#ifndef BPMNOS_Model_RandomDistributionFactory_H
#define BPMNOS_Model_RandomDistributionFactory_H

#include <string>
#include <random>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>
#include <limex.h>

namespace BPMNOS {

using RandomGenerator = std::mt19937;
using RandomDistribution = std::function<double(RandomGenerator &)>;

template <class DistributionType, class... Parameters>
RandomDistribution make_distribution_impl(nlohmann::json const &input, Parameters... parameters) {
	return DistributionType{input.at(parameters)...};
}

RandomDistribution make_distribution(const std::string& jsonString);
RandomDistribution make_distribution(const nlohmann::json& input);

/**
 * @brief Factory for random distribution functions in LIMEX expressions.
 *
 * Registers random functions (uniform, normal, etc.) with a LIMEX handle.
 * Before evaluating expressions containing random functions, setCurrentRng()
 * must be called to provide the RNG context.
 */
class RandomDistributionFactory {
public:
  /**
   * @brief Register all random distribution functions with the given LIMEX handle.
   *
   * Supported functions:
   * - uniform(min, max): Uniform real distribution
   * - uniform_int(min, max): Uniform integer distribution
   * - normal(mean, stddev): Normal/Gaussian distribution
   * - exponential(rate): Exponential distribution
   * - poisson(mean): Poisson distribution
   * - bernoulli(p): Bernoulli (0 or 1)
   * - binomial(n, p): Binomial distribution
   * - gamma(shape, scale): Gamma distribution
   * - lognormal(logscale, shape): Log-normal distribution
   * - geometric(p): Geometric distribution
   */
  void registerFunctions(LIMEX::Handle<double>& handle);

  /**
   * @brief Set the current RNG to use for expression evaluation.
   *
   * Must be called before evaluating any expression containing random functions.
   * @param rng Pointer to the RNG to use, or nullptr to clear.
   */
  void setCurrentRng(std::mt19937* rng) { currentRng = rng; }

  /**
   * @brief Get the current RNG.
   */
  std::mt19937* getCurrentRng() const { return currentRng; }

private:
  std::mt19937* currentRng = nullptr;
};

} // namespace BPMNOS

#endif // BPMNOS_Model_RandomDistributionFactory_H
