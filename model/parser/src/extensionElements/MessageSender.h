#ifndef BPMNOS_MessageSender_H
#define BPMNOS_MessageSender_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Message.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Numeric.h"

namespace BPMNOS {

class MessageSender : public Message {
public:
  MessageSender(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::optional< std::unique_ptr<Parameter> > recipient; ///< Parameter indicating the receiving instance.
  std::vector< BPMN::CatchEvent* > allowedRecipients; ///< List of all potential recipients of the message.

  template <typename T>
  std::unordered_map< std::string, std::optional<std::string> > send(const std::vector<std::optional<T> >& values) const {
    std::unordered_map< std::string, std::optional<std::string> > message;

    for ( auto& content : contents ) {
      if ( content->attribute.has_value() && values[content->attribute->get().index].has_value() ) {
        switch ( content->attribute->get().index ) {
          case Attribute::Type::STRING:
            message[content->key] = stringRegistry[values[content->attribute->get().index].value()];
            break;
          case Attribute::Type::BOOLEAN:
            message[content->key] = stringRegistry[values[content->attribute->get().index].value()];
            break;
          case Attribute::Type::INTEGER:
            message[content->key] = std::to_string(values[content->attribute->get().index].value());
            break;
          case Attribute::Type::DECIMAL:
            message[content->key] = std::to_string(values[content->attribute->get().index].value());
            break;
        }
      }
      else if ( content->value.has_value() ) {
        message[content->key] = content->value->get().value;
      }
      message[content->key] = std::nullopt;
    }
    return message;
  }
};

} // namespace BPMNOS

#endif // BPMNOS_MessageSender_H
