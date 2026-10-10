#ifndef BPMNOS_Execution_ContentMap_H
#define BPMNOS_Execution_ContentMap_H

#include <string>
#include <unordered_map>
#include <variant>
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

/**
 * @brief The content a message or signal carries: for each key, the value of the sender's attribute, or a text
 * that content raised from outside the model states, which is converted to the type of the receiving attribute.
 */
typedef std::unordered_map< std::string, std::variant< Value, std::string > > ContentMap;

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ContentMap_H
