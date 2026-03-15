#include "ExpectedValueFactory.h"
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
}
