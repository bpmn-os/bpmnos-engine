#ifndef BPMNOS_Execution_ContentMap_H
#define BPMNOS_Execution_ContentMap_H

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include "model/utility/src/Value.h"
#include "model/utility/src/Object.h"
#include "model/bpmnos/src/extensionElements/AttributeRegistry.h"

namespace BPMNOS::Execution {

/**
 * @brief The content a message or signal carries: for each key, the value of the sender's attribute, the array
 * or object the sender's object attribute holds, shared until either side writes it, or a text that content
 * raised from outside the model states, which is converted to the type of the receiving attribute.
 */
typedef std::unordered_map< std::string, std::variant< Value, std::string, std::shared_ptr<const Object> > > ContentMap;

/**
 * @brief Writes the content received under a key to the receiving attribute and returns the change of the
 * objective.
 *
 * A value is assigned to a scalar attribute, and an absent key leaves it undefined. An array or object is
 * assigned to an object attribute as a whole assignment is, padded at fixed dimensions and taking the lengths of
 * open ones, and an absent key leaves the object as it is. A text is converted to the type of the receiving
 * attribute, a text for an object attribute being a literal. An array or object sent to a scalar attribute, and a
 * value sent to an object attribute, are errors.
 */
template <typename DataType>
BPMNOS::number applyContent(const BPMNOS::Model::AttributeRegistry& attributeRegistry, const std::string& key, const BPMNOS::Model::Attribute* attribute, const ContentMap::mapped_type* content, BPMNOS::Status& status, DataType& data);

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ContentMap_H
