#include "ExpectedValueScenario.h"

using namespace BPMNOS::Model;

ExpectedValueScenario::ExpectedValueScenario(
  const Model* model,
  BPMNOS::number earliestInstantiationTime,
  BPMNOS::number latestInstantiationTime,
  const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap
)
  : StaticScenario(model, earliestInstantiationTime, latestInstantiationTime, globalValueMap)
{
}
