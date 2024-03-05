#ifndef BPMNOS_Model_StringExpression_H
#define BPMNOS_Model_StringExpression_H

#include <vector>
#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class allowing to compare strings.
 **/
class StringExpression : public Expression {
public:
  StringExpression(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap);
 
  enum class Type { EQUAL, NOTEQUAL };
  Type type;
  Attribute* attribute;
  std::variant<const Attribute*, BPMNOS::number > rhs;

/**
 * @brief Checks whether the attribute matches a given string.
 */
  std::optional<BPMNOS::number> execute(const Values& values) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StringExpression_H
