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

  std::vector<Attribute*> statusAttributes; ///< The scalar status attributes, numbered by their position
  std::vector<Attribute*> dataAttributes; ///< The scalar data attributes, the global attributes of the model first
  std::vector<Attribute*> statusObjects; ///< The status objects, numbered by their position, separately from scalars
  std::vector<Attribute*> dataObjects; ///< The data objects, the global objects of the model first
  /// The schema of the result of each lookup returning an array, by its name, which an expression may only
  /// assign to an array
  std::unordered_map< std::string, const Schema*> arrayLookups;
  std::unordered_map< std::string, Attribute*> statusMap;
  std::unordered_map< std::string, Attribute*> dataMap;
  Attribute* operator[](const std::string& name) const;
  bool contains(const std::string& name) const;
  bool contains(const Attribute* attribute) const;

  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Status& status, const Data& data) const;
  std::optional<BPMNOS::number> getValue(const Attribute* attribute, const Status& status, const SharedData& data) const;
  /// @brief Method returning the object an object attribute holds in the status or data.
  const std::shared_ptr<const Object>& getObject(const Attribute* object, const Status& status, const Data& data) const;
  /// @copydoc getObject(const Attribute*, const Status&, const Data&) const
  const std::shared_ptr<const Object>& getObject(const Attribute* object, const Status& status, const SharedData& data) const;
  /// @brief Method returning the slot holding the object of an object attribute in the status or data, for
  /// writing it.
  std::shared_ptr<const Object>& getObjectSlot(const Attribute* object, Status& status, Data& data) const;
  /// @copydoc getObjectSlot(const Attribute*, Status&, Data&) const
  std::shared_ptr<const Object>& getObjectSlot(const Attribute* object, Status& status, SharedData& data) const;
  /**
   * @brief Method assigning an array or object to an object attribute as a whole.
   *
   * The object keeps the lengths of its fixed dimensions, a shorter value being padded with undefined values and a
   * longer one being an error, and takes the lengths of its open ones. A value of the very layout is shared.
   */
  void setObject(const Attribute* object, Status& status, Data& data, const std::shared_ptr<const Object>& value) const;
  /// @copydoc setObject(const Attribute*, Status&, Data&, const std::shared_ptr<const Object>&) const
  void setObject(const Attribute* object, Status& status, SharedData& data, const std::shared_ptr<const Object>& value) const;
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
