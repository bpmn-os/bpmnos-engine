#include "ExpectedValueFactory.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

using namespace BPMNOS::Execution;

void ExpectedValueFactory::registerFunctions(LIMEX::Handle<double>& handle) {
  auto nameExists = [&](const std::string& name) {
    return std::ranges::contains(handle.getFunctionNames(), name) ||
           std::ranges::contains(handle.getAggregatorNames(), name);
  };

  // uniform(min, max) - Expected value: (min + max) / 2
  if (!nameExists("uniform")) {
    handle.addFunction("uniform", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform requires 2 arguments: min, max");
      }
      return (args[0] + args[1]) / 2.0;
    });
  }

  // uniform_int(min, max) - Expected value: (min + max) / 2
  if (!nameExists("uniform_int")) {
    handle.addFunction("uniform_int", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("uniform_int requires 2 arguments: min, max");
      }
      return (args[0] + args[1]) / 2.0;
    });
  }

  // normal(mean, stddev) - Expected value: mean
  if (!nameExists("normal")) {
    handle.addFunction("normal", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("normal requires 2 arguments: mean, stddev");
      }
      return args[0];
    });
  }

  // exponential(rate) - Expected value: 1 / rate
  if (!nameExists("exponential")) {
    handle.addFunction("exponential", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("exponential requires 1 argument: rate");
      }
      return 1.0 / args[0];
    });
  }

  // poisson(mean) - Expected value: mean
  if (!nameExists("poisson")) {
    handle.addFunction("poisson", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("poisson requires 1 argument: mean");
      }
      return args[0];
    });
  }

  // bernoulli(p) - Expected value: p
  if (!nameExists("bernoulli")) {
    handle.addFunction("bernoulli", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("bernoulli requires 1 argument: probability");
      }
      return args[0];
    });
  }

  // binomial(n, p) - Expected value: n * p
  if (!nameExists("binomial")) {
    handle.addFunction("binomial", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("binomial requires 2 arguments: trials, probability");
      }
      return args[0] * args[1];
    });
  }

  // gamma(shape, scale) - Expected value: shape * scale
  if (!nameExists("gamma")) {
    handle.addFunction("gamma", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("gamma requires 2 arguments: shape, scale");
      }
      return args[0] * args[1];
    });
  }

  // lognormal(logscale, shape) - Expected value: exp(logscale + shape^2 / 2)
  if (!nameExists("lognormal")) {
    handle.addFunction("lognormal", [](const std::vector<double>& args) -> double {
      if (args.size() != 2) {
        throw std::runtime_error("lognormal requires 2 arguments: logscale, shape");
      }
      return std::exp(args[0] + args[1] * args[1] / 2.0);
    });
  }

  // geometric(p) - Expected value: (1 - p) / p
  if (!nameExists("geometric")) {
    handle.addFunction("geometric", [](const std::vector<double>& args) -> double {
      if (args.size() != 1) {
        throw std::runtime_error("geometric requires 1 argument: probability");
      }
      return (1.0 - args[0]) / args[0];
    });
  }

  // triangular(min, mode, max) - Expected value: (min + mode + max) / 3
  if (!nameExists("triangular")) {
    handle.addFunction("triangular", [](const std::vector<double>& args) -> double {
      if (args.size() != 3) {
        throw std::runtime_error("triangular requires 3 arguments: min, mode, max");
      }
      return (args[0] + args[1] + args[2]) / 3.0;
    });
  }

  // discrete(values, probabilities) - Mode: value with highest probability
  if (!nameExists("discrete")) {
    handle.addAggregator("discrete", 2, [](const std::vector<LIMEX::View<double>>& arrays) -> double {
      auto& values = arrays[0];
      auto& probabilities = arrays[1];
      if (values.size() != probabilities.size()) {
        throw std::runtime_error("discrete: values and probabilities must have the same size");
      }
      if (values.size() == 0) {
        throw std::runtime_error("discrete: values and probabilities must not be empty");
      }
      size_t mode = 0;
      for (size_t i = 0; i < values.size(); i++) {
        if (std::isnan(values[i]) || std::isnan(probabilities[i])) {
          throw std::runtime_error("discrete: values and probabilities must be defined");
        }
        if (probabilities[i] > probabilities[mode]) {
          mode = i;
        }
      }
      return values[mode];
    });
  }
}
