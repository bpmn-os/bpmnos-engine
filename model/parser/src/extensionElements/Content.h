#ifndef BPMNOS_Content_H
#define BPMNOS_Content_H

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <bpmn++.h>
#include "model/parser/src/xml/bpmnos/tContent.h"
#include "Attribute.h"

namespace BPMNOS {

class Content;

typedef std::unordered_map<std::string, Content*> ContentMap;

class Content {
public:
  Content(XML::bpmnos::tContent* content, AttributeMap& attributeMap);
  XML::bpmnos::tContent* element;

  std::string& id;
  std::string& key;
  std::optional< std::reference_wrapper<Attribute> > attribute;
  std::optional< std::reference_wrapper<XML::Value> > value;

protected:
  std::optional< std::reference_wrapper<Attribute> > getAttribute(AttributeMap& attributeMap);
};

typedef std::unordered_map< std::string, std::unique_ptr<Content> > contentMap;

} // namespace BPMNOS

#endif // BPMNOS_Content_H
