#ifndef BPMNOS_Model_Deferral_H
#define BPMNOS_Model_Deferral_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "model/utility/src/Value.h"
#include "model/bpmnos/src/xml/bpmnos/tDeferral.h"
#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Expression.h"

namespace BPMNOS::Model {

/**
 * @brief The deferral of a decision point, permitting a controller to defer the decisions of the decision point.
 *
 * A deferral is stated in the message guidance of a catch event, in the choice guidance of a decision task, or in the
 * extension elements of a `performer` element for the entries of the sequential activities of the performer. Its
 * condition is a boolean expression deciding whether deferring is allowed, evaluated on the token of the decision
 * point. Its observed attributes, each stated by a parameter named `observable` whose value is the name of a data or
 * global attribute, `time` included, are the attributes whose update ends a deferral.
 */
class Deferral {
public:
  Deferral(XML::bpmnos::tDeferral* deferral, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tDeferral* element;

  const Expression condition; ///< The condition under which deferring is allowed, whose inputs are the attributes it reads
  std::vector<const Attribute*> observables; ///< The attributes whose update ends a deferral, in the order stated
  const bool timeDependent; ///< True if the condition reads `time` or `time` is observed, so that the passage of time alone may end a deferral

  /**
   * @brief Returns true if the condition holds for the given status and data.
   *
   * A condition whose value is undefined does not hold.
   */
  template <typename DataType>
  bool conditionHolds(const BPMNOS::Status& status, const DataType& data) const;
private:
  static std::vector<const Attribute*> getObservables(XML::bpmnos::tDeferral* deferral, const AttributeRegistry& attributeRegistry);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Deferral_H
