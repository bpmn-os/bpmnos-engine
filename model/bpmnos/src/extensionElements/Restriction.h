#ifndef BPMNOS_Model_Restriction_H
#define BPMNOS_Model_Restriction_H

#include <memory>
#include <unordered_set>
#include <string>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Expression.h"

namespace BPMNOS::Model {

class Restriction {
public:
  Restriction(XML::bpmnos::tRestriction* restriction, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tRestriction* element;

  std::string& id;
  const Expression expression;

  enum class Scope { ENTRY, EXIT, FULL };
  Scope scope;

/**
 * @brief Check if the restriction is satisfied using an expression applied on status and data attribute values.
 *
 * This function checks whether the given restriction is satisfied based on the
 * provided status and data attribute values. 
 *
 * @return `true` if the restriction is satisfied, `false` otherwise.
 */
  template <typename DataType>
  bool isSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Restriction_H
