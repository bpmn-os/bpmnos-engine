#ifndef BPMNOS_Execution_Content_H
#define BPMNOS_Execution_Content_H

#include <memory>
#include "model/utility/src/Number.h"
#include "model/utility/src/Object.h"

namespace BPMNOS::Execution {

/**
 * @brief The content of a message or signal: its name together with its content object.
 *
 * An object is a value, shared copy-on-write among everything holding it, and carries no name; the name is that
 * of the message or signal, which the definition stating it gives, or the environment raising it.
 */
struct Content {
  BPMNOS::number name; ///< The name of the message or signal
  std::shared_ptr<const BPMNOS::Object> object; ///< The content object, shared until written
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Content_H
