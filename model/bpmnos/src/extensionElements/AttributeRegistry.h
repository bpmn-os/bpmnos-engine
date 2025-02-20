#ifndef BPMNOS_Model_AttributeRegistry_H
#define BPMNOS_Model_AttributeRegistry_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <limex.h>
#include "model/utility/src/Number.h"

#include "Attribute.h"

namespace BPMNOS::Model {

class AttributeRegistry {
public:
  AttributeRegistry(const LIMEX::Callables<double>& callables);
  const LIMEX::Callables<double>& callables;

  std::map< std::string, Attribute*> statusAttributes;
  std::map< std::string, Attribute*> dataAttributes;
  std::map< std::string, Attribute*> globalAttributes;
  Attribute* operator[](const std::string& name) const;
  bool contains(const std::string& name) const;

  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Values& status, const Values& data, const Values& globals) const;
  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Values& status, const SharedValues& data, const Values& globals) const;
  void setValue(const Attribute* attribute, Values& status, Values& data, Values& globals, std::optional<BPMNOS::number> value) const;
  void setValue(const Attribute* attribute, Values& status, SharedValues& data, Values& globals, std::optional<BPMNOS::number> value) const;
private:
  friend class Attribute;
  void add(Attribute* attribute);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AttributeRegistry_H
