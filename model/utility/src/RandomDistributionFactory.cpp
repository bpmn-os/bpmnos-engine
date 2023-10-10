#include "RandomDistributionFactory.h"
#include <exception>

using namespace BPMNOS;

RandomDistribution BPMNOS::make_distribution(const std::string& jsonString) {
  nlohmann::json input = nlohmann::json::parse(jsonString);
  return make_distribution(input);
}

RandomDistribution BPMNOS::make_distribution(const nlohmann::json& input) {
	const std::string& distributionName = input.at("distribution");
	
  // https://en.cppreference.com/w/cpp/numeric/random/
  // all distributions implement min() and max()

  if (distributionName == "uniform_int_distribution") {
    //  Produces integer values evenly distributed across a given range. 
    return make_distribution_impl<std::uniform_int_distribution<int>>(input, "min", "max");
  }

  if (distributionName == "uniform_real_distribution") {
    //  Produces real values evenly distributed across a given range. 
    return make_distribution_impl<std::uniform_real_distribution<double>>(input, "min", "max");
  }

  if (distributionName == "bernoulli_distribution") {
    //  Produces bool values with a given probability of a true outcome.
    return make_distribution_impl<std::bernoulli_distribution>(input, "probability");
  }

  if (distributionName == "binomial_distribution") {
    //  The result obtained is the number of successes in a given number of trials, each of which succeeds with a given probability. 
    return make_distribution_impl<std::binomial_distribution<int>>(input, "trials", "probability");
  }

  if (distributionName == "negative_binomial_distribution") {
    //  The results represents the number of failures in a series of independent yes/no trials (each succeeds with a given probability), before exactly the given number of successes occur. 
    return make_distribution_impl<std::negative_binomial_distribution<int>>(input, "successes", "probability");
  }

  if (distributionName == "geometric_distribution") {
    // The results represents the number of failures in a series of independent yes/no trials (each succeeds with probability p), before exactly 1 success occurs. 
    return make_distribution_impl<std::geometric_distribution<int>>(input, "probability");
  }

  if (distributionName == "poisson_distribution") {
    // The result obtained is the number of occurrences of a random event for a given mean indicating the number of its occurrences under the same conditions (on the same time/space interval).
    return make_distribution_impl<std::poisson_distribution<int>>(input, "mean");
  }

  if (distributionName == "exponential_distribution") {
    // The result obtained is the time/distance until the next random event if random events occur at given rate per unit of time/distance.
    return make_distribution_impl<std::exponential_distribution<double>>(input, "rate");
  }

  if (distributionName == "gamma_distribution") {
    // For shape parameter α, the value obtained is the sum of α independent exponentially distributed random variables, each of which has a mean of the scale parameter β. 
    return make_distribution_impl<std::gamma_distribution<double>>(input, "shape", "scale");
  }

  // weibull_distribution and extreme_value_distribution skipped

  if (distributionName == "normal_distribution") {
    // Generates random numbers according to the Normal (or Gaussian) random number distribution with given mean and standard deviation.
    return make_distribution_impl<std::normal_distribution<double>>(input, "mean", "stddev");
  }

  if (distributionName == "lognormal_distribution") {
    // Generates random numbers according to the Log-normal random number distribution with given log-scale and shape parameter.
    return make_distribution_impl<std::normal_distribution<double>>(input, "log-scale", "shape");
  }

  // chi_squared_distribution, cauchy_distribution, fisher_f_distribution, and student_t_distribution skipped

  if (distributionName == "discrete_distribution") {
    // Produces a result with probabilities based on the given weights.
    return make_distribution_impl<std::discrete_distribution<int>>(input, "weights");
    // result must be linked to outcomes (default outcomes are 0, 1, ..., n where n is the number of weights)
  }

  // piecewise_constant_distribution and piecewise_linear_distribution skipped because 
  // they require input manipulation to obtain iterators

  throw std::runtime_error("Unknown distribution " + distributionName);
}

