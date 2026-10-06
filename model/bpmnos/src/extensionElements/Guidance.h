#ifndef BPMNOS_Model_Guidance_H
#define BPMNOS_Model_Guidance_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Parameter.h"
#include "Restriction.h"
#include "Operator.h"
#include "model/bpmnos/src/xml/bpmnos/tGuidance.h"

namespace BPMNOS::Model {


class Guidance {
public:
  Guidance(XML::bpmnos::tGuidance* guidance, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tGuidance* element;
  AttributeRegistry attributeRegistry; ///< Registry allowing to look up attributes by their names.

  enum class Type { Entry, Exit, Choice, MessageDelivery };
  Type type;
  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;
  
  std::set<const Attribute*> dependencies;

  /**
   * @brief Method appending the attributes of the guidance to the status and applying the operators.
   *
   * The attributes take only the values the model assigns to them, so each is appended undefined, or with
   * the value of its expression if it has one.
   */
  template <typename DataType>
  void apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const;

  template <typename DataType>
  BPMNOS::number getObjective(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;

  template <typename DataType>
  bool restrictionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Guidance_H
