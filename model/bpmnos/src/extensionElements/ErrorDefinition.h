#ifndef BPMNOS_Model_ErrorDefinition_H
#define BPMNOS_Model_ErrorDefinition_H

#include "model/utility/src/Schema.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/xml/bpmnos/tError.h"

namespace BPMNOS::Model {

/**
 * @brief The definition an error states: its name and the type of its content, which has no header.
 */
class ErrorDefinition {
public:
  ErrorDefinition(XML::bpmnos::tError* error);
  XML::bpmnos::tError* element;
  BPMNOS::number name; ///< Error name
  Schema schema = StructType{}; ///< The type of the content, a struct, without fields if the error states no type
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ErrorDefinition_H
