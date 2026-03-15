#include "RandomDistributionFactory.h"
#include <exception>
#include <algorithm>

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

void RandomDistributionFactory::registerFunctions(LIMEX::Handle<double>& handle) {
  const auto& existingNames = handle.getNames();
  auto nameExists = [&](const std::string& name) {
    return std::find(existingNames.begin(), existingNames.end(), name) != existingNames.end();
  };

  // uniform(min, max) - Uniform real distribution
  if (!nameExists("uniform")) {
    handle.add("uniform", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform requires 2 arguments: min, max");
      }
      if (!currentRng) {
        throw std::runtime_error("uniform: no RNG context set");
      }
      std::uniform_real_distribution<double> dist(args[0], args[1]);
      return dist(*currentRng);
    });
  }

  // uniform_int(min, max) - Uniform integer distribution
  if (!nameExists("uniform_int")) {
    handle.add("uniform_int", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform_int requires 2 arguments: min, max");
      }
      if (!currentRng) {
        throw std::runtime_error("uniform_int: no RNG context set");
      }
      std::uniform_int_distribution<int> dist(static_cast<int>(args[0]), static_cast<int>(args[1]));
      return static_cast<double>(dist(*currentRng));
    });
  }

  // normal(mean, stddev) - Normal/Gaussian distribution
  if (!nameExists("normal")) {
    handle.add("normal", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("normal requires 2 arguments: mean, stddev");
      }
      if (!currentRng) {
        throw std::runtime_error("normal: no RNG context set");
      }
      std::normal_distribution<double> dist(args[0], args[1]);
      return dist(*currentRng);
    });
  }

  // exponential(rate) - Exponential distribution
  if (!nameExists("exponential")) {
    handle.add("exponential", [this](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("exponential requires 1 argument: rate");
      }
      if (!currentRng) {
        throw std::runtime_error("exponential: no RNG context set");
      }
      std::exponential_distribution<double> dist(args[0]);
      return dist(*currentRng);
    });
  }

  // poisson(mean) - Poisson distribution
  if (!nameExists("poisson")) {
    handle.add("poisson", [this](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("poisson requires 1 argument: mean");
      }
      if (!currentRng) {
        throw std::runtime_error("poisson: no RNG context set");
      }
      std::poisson_distribution<int> dist(args[0]);
      return static_cast<double>(dist(*currentRng));
    });
  }

  // bernoulli(p) - Bernoulli distribution (returns 0 or 1)
  if (!nameExists("bernoulli")) {
    handle.add("bernoulli", [this](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("bernoulli requires 1 argument: probability");
      }
      if (!currentRng) {
        throw std::runtime_error("bernoulli: no RNG context set");
      }
      std::bernoulli_distribution dist(args[0]);
      return dist(*currentRng) ? 1.0 : 0.0;
    });
  }

  // binomial(n, p) - Binomial distribution
  if (!nameExists("binomial")) {
    handle.add("binomial", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("binomial requires 2 arguments: trials, probability");
      }
      if (!currentRng) {
        throw std::runtime_error("binomial: no RNG context set");
      }
      std::binomial_distribution<int> dist(static_cast<int>(args[0]), args[1]);
      return static_cast<double>(dist(*currentRng));
    });
  }

  // gamma(shape, scale) - Gamma distribution
  if (!nameExists("gamma")) {
    handle.add("gamma", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("gamma requires 2 arguments: shape, scale");
      }
      if (!currentRng) {
        throw std::runtime_error("gamma: no RNG context set");
      }
      std::gamma_distribution<double> dist(args[0], args[1]);
      return dist(*currentRng);
    });
  }

  // lognormal(logscale, shape) - Log-normal distribution
  if (!nameExists("lognormal")) {
    handle.add("lognormal", [this](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("lognormal requires 2 arguments: logscale, shape");
      }
      if (!currentRng) {
        throw std::runtime_error("lognormal: no RNG context set");
      }
      std::lognormal_distribution<double> dist(args[0], args[1]);
      return dist(*currentRng);
    });
  }

  // geometric(p) - Geometric distribution
  if (!nameExists("geometric")) {
    handle.add("geometric", [this](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("geometric requires 1 argument: probability");
      }
      if (!currentRng) {
        throw std::runtime_error("geometric: no RNG context set");
      }
      std::geometric_distribution<int> dist(args[0]);
      return static_cast<double>(dist(*currentRng));
    });
  }
}
