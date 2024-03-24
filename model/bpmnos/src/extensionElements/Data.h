#ifndef BPMNOS_Model_Data_H
#define BPMNOS_Model_Data_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"

namespace BPMNOS::Model {

/**
 * @brief Class holding extension elements for data objects 
 **/
class Data : public BPMN::ExtensionElements {
public:
  Data(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* scope);
  const BPMN::Scope* scope;
  AttributeMap attributeMap; ///< Map allowing to look up all attributes by their names.
  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing attributes declared for the data object.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Data_H
