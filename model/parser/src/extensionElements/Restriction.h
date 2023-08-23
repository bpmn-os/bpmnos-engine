#ifndef BPMNOS_Restriction_H
#define BPMNOS_Restriction_H

#include <memory>
#include <unordered_set>
#include <string>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "Attribute.h"

namespace BPMNOS::Model {

class Restriction {
public:
  Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap);
  XML::bpmnos::tRestriction* element;

  std::string& id;
  Attribute* attribute;

  bool negated;
  bool required;
  std::optional<number> minInclusive;
  std::optional<number> maxInclusive;
  std::unordered_set<number,ValueHash> enumeration;

/**
 * @brief Check if the restriction is satisfied based on status values.
 *
 * This function checks whether the given restriction is satisfied based on the
 * provided status values. The function considers whether the restriction is
 * negated and applies various checks based on the attribute's type and values.
 *
 * The function performs the following checks:
 * - If the restriction is not negated:
 *   - If the restriction is marked as required and the corresponding attribute
 *     is missing from the status values, the restriction is not satisfied.
 *   - For numeric attributes (integer or decimal):
 *     - If a minimum inclusive value is defined and the attribute has a value
 *       in the status that is less than the minimum, the restriction is not satisfied.
 *     - If a maximum inclusive value is defined and the attribute has a value
 *       in the status that is greater than the maximum, the restriction is not satisfied.
 *   - If the restriction defines an enumeration and the attribute has a value
 *     in the status, the value must be present in the enumeration for the restriction
 *     to be satisfied.
 * - If the restriction is negated:
 *   - If the restriction is marked as required and the corresponding attribute
 *     has a value in the status, the restriction is not satisfied.
 *   - For numeric attributes (integer or decimal):
 *     - If a minimum inclusive value is defined and the attribute has a value
 *       in the status that is greater than or equal to the minimum, the restriction
 *       is not satisfied.
 *     - If a maximum inclusive value is defined and the attribute has a value
 *       in the status that is less than or equal to the maximum, the restriction
 *       is not satisfied.
 *   - If the restriction defines an enumeration and the attribute has a value
 *     in the status, the value must not be present in the enumeration for the
 *     restriction to be satisfied.
 *
 * The function returns `true` if all the specified checks are passed and the
 * restriction is considered satisfied.
 *
 * @param status The status values to be evaluated against the restriction.
 * @return `true` if the restriction is satisfied, `false` otherwise.
 */
  bool isSatisfied(const Values& status) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Restriction_H
