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
  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;

  inline std::size_t size() const { return parentSize + attributes.size(); };

  template <typename T>
  bool isFeasible(const std::vector<std::optional<T> >& values) const {
    for ( auto restriction : restrictions ) {
      if ( !restriction->isSatisfied(values) ) {
        return false; 
      }
    }
    return true; 
  }  

  template <typename T>
  void applyOperators(std::vector<std::optional<T> >& values) const {
    for ( auto operator_ : operators ) {
      operator_->apply(values);
    }
  }

protected:
  std::size_t parentSize;
  AttributeMap attributeMap; ///< Map allowing to look up attributes by their names.
};

} // namespace BPMNOS

#endif // BPMNOS_Status_H
