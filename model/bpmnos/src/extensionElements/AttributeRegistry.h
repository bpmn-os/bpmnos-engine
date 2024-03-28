#ifndef BPMNOS_Model_AttributeRegistry_H
#define BPMNOS_Model_AttributeRegistry_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "model/utility/src/Number.h"

#include "Attribute.h"

namespace BPMNOS::Model {

class AttributeRegistry {
public:
  std::map< std::string, Attribute*> statusAttributes;
  std::map< std::string, Attribute*> dataAttributes;
  Attribute* operator[](const std::string& name) const;
  bool contains(const std::string& name) const;
  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Values& status, const Values& data) const;
  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Values& status, const Globals& data) const;
  void setValue(const Attribute* attribute, Values& status, Values& data, std::optional<BPMNOS::number> value) const;
  void setValue(const Attribute* attribute, Values& status, Globals& data, std::optional<BPMNOS::number> value) const;
private:
  friend class Attribute;
  void add(Attribute* attribute);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AttributeRegistry_H
