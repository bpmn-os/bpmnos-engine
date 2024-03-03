#ifndef BPMNOS_Model_Restriction_H
#define BPMNOS_Model_Restriction_H

#include <memory>
#include <unordered_set>
#include <string>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "Attribute.h"
#include "Expression.h"

namespace BPMNOS::Model {

class Restriction {
public:
  Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap);
  XML::bpmnos::tRestriction* element;

  std::string& id;
  std::unique_ptr<Expression> expression;

  enum class Scope { ENTRY, EXIT, FULL };
  Scope scope;

/**
 * @brief Check if the restriction is satisfied using an expression applied on status values.
 *
 * This function checks whether the given restriction is satisfied based on the
 * provided status values. 
 *
 * @param status The status values to be evaluated against the restriction.
 * @return `true` if the restriction is satisfied, `false` otherwise.
 */
  bool isSatisfied(const Values& status) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Restriction_H
