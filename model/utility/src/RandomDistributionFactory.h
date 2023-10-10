#ifndef BPMNOS_RandomDistributionFactory_H
#define BPMNOS_RandomDistributionFactory_H

#include <string>
#include <random>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

namespace BPMNOS {

using RandomGenerator = std::mt19937;
using RandomDistribution = std::function<double(RandomGenerator &)>;

template <class DistributionType, class... Parameters>
RandomDistribution make_distribution_impl(nlohmann::json const &input, Parameters... parameters) {
	return DistributionType{input.at(parameters)...};
}

RandomDistribution make_distribution(const std::string& jsonString);
RandomDistribution make_distribution(const nlohmann::json& input);

} // namespace BPMNOS

#endif // BPMNOS_RandomDistributionFactory_H
