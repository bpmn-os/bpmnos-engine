#include "ExpectedValueFactory.h"
#include "CollectionRegistry.h"
#include <stdexcept>
#include <algorithm>

using namespace BPMNOS;

void ExpectedValueFactory::registerFunctions(LIMEX::Handle<double>& handle) {
  const auto& existingNames = handle.getNames();
  auto nameExists = [&](const std::string& name) {
    return std::find(existingNames.begin(), existingNames.end(), name) != existingNames.end();
  };

  // uniform(min, max) - Expected value: (min + max) / 2
  if (!nameExists("uniform")) {
    handle.add("uniform", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform requires 2 arguments: min, max");
      }
      return (args[0] + args[1]) / 2.0;
    });
  }

  // uniform_int(min, max) - Expected value: (min + max) / 2
  if (!nameExists("uniform_int")) {
    handle.add("uniform_int", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform_int requires 2 arguments: min, max");
      }
      return (args[0] + args[1]) / 2.0;
    });
  }

  // normal(mean, stddev) - Expected value: mean
  if (!nameExists("normal")) {
    handle.add("normal", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("normal requires 2 arguments: mean, stddev");
      }
      return args[0];
    });
  }

  // exponential(rate) - Expected value: 1 / rate
  if (!nameExists("exponential")) {
    handle.add("exponential", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("exponential requires 1 argument: rate");
      }
      return 1.0 / args[0];
    });
  }

  // poisson(mean) - Expected value: mean
  if (!nameExists("poisson")) {
    handle.add("poisson", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("poisson requires 1 argument: mean");
      }
      return args[0];
    });
  }

  // bernoulli(p) - Expected value: p
  if (!nameExists("bernoulli")) {
    handle.add("bernoulli", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("bernoulli requires 1 argument: probability");
      }
      return args[0];
    });
  }

  // binomial(n, p) - Expected value: n * p
  if (!nameExists("binomial")) {
    handle.add("binomial", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("binomial requires 2 arguments: trials, probability");
      }
      return args[0] * args[1];
    });
  }

  // gamma(shape, scale) - Expected value: shape * scale
  if (!nameExists("gamma")) {
    handle.add("gamma", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("gamma requires 2 arguments: shape, scale");
      }
      return args[0] * args[1];
    });
  }

  // lognormal(logscale, shape) - Expected value: exp(logscale + shape^2 / 2)
  if (!nameExists("lognormal")) {
    handle.add("lognormal", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("lognormal requires 2 arguments: logscale, shape");
      }
      return std::exp(args[0] + args[1] * args[1] / 2.0);
    });
  }

  // geometric(p) - Expected value: (1 - p) / p
  if (!nameExists("geometric")) {
    handle.add("geometric", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("geometric requires 1 argument: probability");
      }
      return (1.0 - args[0]) / args[0];
    });
  }

  // triangular(min, mode, max) - Expected value: (min + mode + max) / 3
  if (!nameExists("triangular")) {
    handle.add("triangular", [](const std::vector<double>& args) -> double {
      if (args.size() != 3) {
        throw std::runtime_error("triangular requires 3 arguments: min, mode, max");
      }
      return (args[0] + args[1] + args[2]) / 3.0;
    });
  }

  // discrete(values, probabilities) - Mode: value with highest probability
  if (!nameExists("discrete")) {
    handle.add("discrete", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("discrete requires 2 arguments: values collection, probabilities collection");
      }
      size_t valuesIndex = static_cast<size_t>(args[0]);
      size_t probsIndex = static_cast<size_t>(args[1]);

      const auto& values = collectionRegistry[valuesIndex];
      const auto& probs = collectionRegistry[probsIndex];

      if (values.size() != probs.size()) {
        throw std::runtime_error("discrete: values and probabilities must have the same size");
      }
      if (values.empty()) {
        throw std::runtime_error("discrete: collections must not be empty");
      }

      auto maxIt = std::max_element(probs.begin(), probs.end());
      auto maxIndex = static_cast<size_t>(std::distance(probs.begin(), maxIt));
      return values[maxIndex];
    });
  }
}
