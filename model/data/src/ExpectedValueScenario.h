#ifndef BPMNOS_Model_ExpectedValueScenario_H
#define BPMNOS_Model_ExpectedValueScenario_H

#include "StaticScenario.h"

namespace BPMNOS::Model {

class ExpectedValueDataProvider;

/**
 * @brief A scenario identical to StaticScenario for expected value evaluation.
 *
 * ExpectedValueScenario is used with ExpectedValueDataProvider where:
 * - INITIALIZATION expressions use expected value functions
 * - DISCLOSURE is ignored (all values disclosed at time 0)
 * - COMPLETION is ignored (operators can be used to compute expected values during execution)
 */
class ExpectedValueScenario : public StaticScenario {
  friend class ExpectedValueDataProvider;

public:
  ExpectedValueScenario(
    const Model* model,
    BPMNOS::number earliestInstantiationTime,
    BPMNOS::number latestInstantiationTime,
    const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap
  );
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpectedValueScenario_H
