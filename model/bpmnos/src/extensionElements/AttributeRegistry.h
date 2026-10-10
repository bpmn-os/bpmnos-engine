#ifndef BPMNOS_Model_AttributeRegistry_H
#define BPMNOS_Model_AttributeRegistry_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <limex.h>
#include "model/utility/src/Value.h"

#include "Attribute.h"

namespace BPMNOS::Model {

class AttributeRegistry {
public:
  AttributeRegistry(const LIMEX::Handle<double>& limexHandle);
  const LIMEX::Handle<double>& limexHandle;

  std::vector<Attribute*> statusAttributes;
  std::vector<Attribute*> dataAttributes; ///< The data attributes, the global attributes of the model first
  std::unordered_map< std::string, Attribute*> statusMap;
  std::unordered_map< std::string, Attribute*> dataMap;
  Attribute* operator[](const std::string& name) const;
  bool contains(const std::string& name) const;
  bool contains(const Attribute* attribute) const;

  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Status& status, const Data& data) const;
  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Status& status, const SharedData& data) const;
  /// @brief Method setting the value of an attribute and returning the change of the objective, which is the
  /// change of the value times the weight of the attribute for a data attribute and zero for a status attribute,
  /// the previous value being available here and nowhere later.
  BPMNOS::number setValue(const Attribute* attribute, Status& status, Data& data, std::optional<BPMNOS::number> value) const;
  /// @copydoc setValue(const Attribute*, Status&, Data&, std::optional<BPMNOS::number>) const
  BPMNOS::number setValue(const Attribute* attribute, Status& status, SharedData& data, std::optional<BPMNOS::number> value) const;
private:
  friend class Attribute;
  void add(Attribute* attribute);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AttributeRegistry_H
