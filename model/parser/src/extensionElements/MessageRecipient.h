#ifndef BPMNOS_MessageRecipient_H
#define BPMNOS_MessageRecipient_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Message.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Number.h"

namespace BPMNOS {

class MessageRecipient : public Message {
public:
  MessageRecipient(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::optional< std::unique_ptr<Parameter> > sender; ///< Parameter indicating the sending instance.
  std::vector< BPMN::ThrowEvent* > allowedSenders; ///< List of all potential senders of the message.

/**
 * @brief Receive and process a message's content to update status values.
 *
 * This function processes the contents of a message and updates the status
 * values based on the content of the message and predefined default values.
 * The function iterates through the contents and updates status values 
 * accordingly:
 *
 * - If the message contains a key and the corresponding value is available,
 *   the status value for the associated attribute is updated with the value
 *   from the message.
 * - If the message doesn't contain a key or the corresponding value is not 
 *   available, but a default value is available in the content, the status 
 *   value for the associated attribute is updated with the default value.
 * - If neither a message value nor a default value is available, the status
 *   value for the associated attribute is set to undefined (std::nullopt).
 *
 * @param status The status values to be updated.
 * @param message The message content containing key-value pairs.
 */
  void receive(Values& status, const ValueMap& message) const;
};

} // namespace BPMNOS

#endif // BPMNOS_MessageRecipient_H
