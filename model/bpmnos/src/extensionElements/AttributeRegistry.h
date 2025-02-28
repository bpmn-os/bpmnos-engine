#ifndef BPMNOS_Model_AttributeRegistry_H
#define BPMNOS_Model_AttributeRegistry_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <limex.h>
#include "model/utility/src/Number.h"

#include "Attribute.h"

namespace BPMNOS::Model {

class AttributeRegistry {
public:
  AttributeRegistry(const LIMEX::Callables<double>& callables);
  const LIMEX::Callables<double>& callables;

  std::vector<Attribute*> statusAttributes;
  std::vector<Attribute*> dataAttributes;
  std::vector<Attribute*> globalAttributes;
  std::unordered_map< std::string, Attribute*> statusMap;
  std::unordered_map< std::string, Attribute*> dataMap;
  std::unordered_map< std::string, Attribute*> globalMap;
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
