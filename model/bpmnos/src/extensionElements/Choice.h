#ifndef BPMNOS_Model_Choice_H
#define BPMNOS_Model_Choice_H

#include <memory>
#include <vector>
#include <variant>
#include <string>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tDecision.h"
#include "Attribute.h"
#include "Expression.h"
#include "AttributeRegistry.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a choice to be made within a @ref BPMNOS::Model::DecisionTask.
 *
 * The choice to be made may be restricted through @ref BPMNOS::Model::Restriction "restrictions". 
 */
class Choice {
public:
  Choice(XML::bpmnos::tDecision* decision, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tDecision* element;
  const AttributeRegistry& attributeRegistry;
  Attribute* attribute;
  std::optional< std::pair<std::unique_ptr<Expression>,bool> > lowerBound; // lower bound and flag indicating strict inequality
  std::optional< std::pair<std::unique_ptr<Expression>,bool> > upperBound; // upper bound and flag indicating strict inequality
  std::unique_ptr<Expression> multipleOf;
  std::vector< std::unique_ptr<Expression> > enumeration;
  std::set<const Attribute*> dependencies; // set of attribute used as input to lower bound, upper bound, or enumeration

  template <typename DataType>
  std::pair<BPMNOS::number,BPMNOS::number> getBounds(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const; ///< Returns the minimal and maximal value the attribute may take.

  template <typename DataType>
  std::vector<BPMNOS::number> getEnumeration(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const; ///< Returns the allowed values the attribute may take.
private:
  void parseEnumeration(const std::string& input);
  void parseBounds(const std::string& input);
  void parseDiscretizer(const std::string& input);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Choice_H
