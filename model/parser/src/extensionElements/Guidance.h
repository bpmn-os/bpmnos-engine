#ifndef BPMNOS_Guidance_H
#define BPMNOS_Guidance_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"
#include "model/parser/src/xml/bpmnos/tGuidance.h"

namespace BPMNOS::Model {


class Guidance {
public:
  Guidance(XML::bpmnos::tGuidance* guidance, AttributeMap attributeMap);
  XML::bpmnos::tGuidance* element;
  AttributeMap attributeMap; ///< Map allowing to look up attributes by their names.

  enum class Type { Message, Entry, Exit, Choice };
  Type type;
  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Guidance_H
