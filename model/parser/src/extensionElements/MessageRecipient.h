#ifndef BPMNOS_MessageRecipient_H
#define BPMNOS_MessageRecipient_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Message.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Numeric.h"

namespace BPMNOS {

class MessageRecipient : public Message {
public:
  MessageRecipient(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::optional< std::unique_ptr<Parameter> > sender; ///< Parameter indicating the sending instance.
  std::vector< BPMN::ThrowEvent* > allowedSenders; ///< List of all potential senders of the message.

  template <typename T>
  void receive(const std::vector<std::optional<T> >& values, const std::unordered_map< std::string, std::optional<std::string> >& message) const {
    for ( auto& content : contents ) {
      std::optional< std::string > value = std::nullopt;
      if ( message.contains(content->key) && message.at(content->key).has_value() ) {
        value = message.at(content->key).value();
      }
      else if ( content->value.has_value() ) {
        value = content->value.value();      
      }
      
      if ( value.has_value() ) {
        // Mimic XML attribute to have consistent type conversion
        XML::Attribute givenValue = {
          .xmlns=content->attribute->get().element->xmlns,
          .prefix=content->attribute->get().element->prefix,
          .name=content->attribute->get().element->name,
          .value = value.value()
        };

        switch ( content->attribute->get().index ) {
          case Attribute::Type::STRING:
            values[content->attribute->get().index] = stringRegistry((std::string)givenValue);
          break;
          case Attribute::Type::BOOLEAN:
            values[content->attribute->get().index] = stringRegistry((std::string)givenValue);
          break;
          case Attribute::Type::INTEGER:
            values[content->attribute->get().index] = (int)givenValue;
          break;
          case Attribute::Type::DECIMAL:
            values[content->attribute->get().index] = numeric<T>((double)givenValue);
          break;
        }
      }
      else {
        values[content->attribute->get().index] = std::nullopt;
      }
    }
  }
};

} // namespace BPMNOS

#endif // BPMNOS_MessageRecipient_H
