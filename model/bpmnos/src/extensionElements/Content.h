#ifndef BPMNOS_Model_Content_H
#define BPMNOS_Model_Content_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tContent.h"
#include "Attribute.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class Content {
public:
  Content(XML::bpmnos::tContent* content, AttributeMap& statusAttributes);
  XML::bpmnos::tContent* element;

  std::string& id;
  std::string& key;
  std::optional< std::reference_wrapper<Attribute> > attribute;
  std::optional< std::string > value; 

protected:
  std::optional< std::reference_wrapper<Attribute> > getAttribute(AttributeMap& statusAttributes);
};

typedef std::unordered_map< std::string, std::unique_ptr<Content> > ContentMap;

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Content_H
