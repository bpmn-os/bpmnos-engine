#ifndef BPMNOS_Status_H
#define BPMNOS_Status_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"

namespace BPMNOS {


class Status : public BPMN::ExtensionElements {
public:
  Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent = nullptr);
  const BPMN::Scope* parent;
  AttributeMap attributeMap; ///< Map allowing to look up attributes by their names.

  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;

  inline std::size_t size() const { return parentSize + attributes.size(); };

  bool isFeasible(const Values& values) const;
  void applyOperators(Values& values) const;

protected:
  std::size_t parentSize;
};

} // namespace BPMNOS

#endif // BPMNOS_Status_H
