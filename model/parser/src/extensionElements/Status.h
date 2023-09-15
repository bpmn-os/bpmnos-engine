#ifndef BPMNOS_Status_H
#define BPMNOS_Status_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"
#include "Decision.h"
#include "Guidance.h"

namespace BPMNOS::Model {


class Status : public BPMN::ExtensionElements {
public:
  Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent = nullptr);
  const BPMN::Scope* parent;
  AttributeMap attributeMap; ///< Map allowing to look up attributes by their names.

  enum Index { Timestamp, Instance }; ///< Indices for timestamp and instance attribute.

  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Decision> > decisions;

  inline std::size_t size() const { return parentSize + attributes.size(); };

  bool isFeasible(const Values& values) const;
  void applyOperators(Values& values) const;
  void makeChoices(const std::unordered_map<Decision*,number>& choices, Values& values) const;

  std::optional< std::unique_ptr<Guidance> > messageGuidance;
  std::optional< std::unique_ptr<Guidance> > entryGuidance;
  std::optional< std::unique_ptr<Guidance> > exitGuidance;
  std::optional< std::unique_ptr<Guidance> > choiceGuidance;
protected:
  std::size_t parentSize;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Status_H
