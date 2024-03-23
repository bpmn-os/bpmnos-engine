#ifndef BPMNOS_Model_Choice_H
#define BPMNOS_Model_Choice_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tChoice.h"
#include "Attribute.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a choice to be made within a @ref BPMNOS::Model::DecisionTask.
 *
 * The choice to be made may be restricted through @ref BPMNOS::Model::Restriction "restrictions". 
 */
class Choice {
public:
  Choice(XML::bpmnos::tChoice* choice, AttributeMap& attributeMap);
  XML::bpmnos::tChoice* element;

  Attribute* attribute;
  BPMNOS::number min;
  BPMNOS::number max;
  
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Choice_H
