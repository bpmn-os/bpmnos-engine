#ifndef BPMNOS_Model_MessageSender_H
#define BPMNOS_Model_MessageSender_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Message.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class MessageSender : public Message {
public:
  MessageSender(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::optional< std::unique_ptr<Parameter> > recipient; ///< Parameter indicating the receiving instance.
  std::vector< BPMN::CatchEvent* > allowedRecipients; ///< List of all potential recipients of the message.

/**
 * @brief Generate a message containing values from the status and defaults.
 *
 * This function generates a message containing values based on the provided
 * status values and predefined default values. The function iterates through
 * the contents and populates the message accordingly:
 *
 * - If the content specifies an attribute and the status contains a value for
 *   that attribute, the value from the status is used in the message.
 * - If the content doesn't specify an attribute or the status doesn't contain
 *   a value for that attribute, but a default value is available in the content,
 *   the default value is used in the message.
 *
 * @param status The status values to be included in the message.
 * @return A map containing key-value pairs representing the generated message.
 */
  BPMNOS::ValueMap send(const BPMNOS::Values& status) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageSender_H
