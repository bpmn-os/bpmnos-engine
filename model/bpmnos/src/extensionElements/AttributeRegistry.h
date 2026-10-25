#ifndef BPMNOS_Model_AttributeRegistry_H
#define BPMNOS_Model_AttributeRegistry_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <limex.h>
#include "model/utility/src/Value.h"

#include "Attribute.h"
#include "model/bpmnos/src/SchemaProvider.h"

namespace BPMNOS::Model {

class Input;
class LookupTable;

class AttributeRegistry {
public:
  AttributeRegistry(const LIMEX::Handle<double>& limexHandle);
  const LIMEX::Handle<double>& limexHandle;

  std::vector<Attribute*> statusAttributes; ///< The scalar status attributes, numbered by their position
  std::vector<Attribute*> dataAttributes; ///< The scalar data attributes, the global attributes of the model first
  std::vector<Attribute*> statusObjects; ///< The status objects, numbered by their position, separately from scalars
  std::vector<Attribute*> dataObjects; ///< The data objects, the global objects of the model first
  /// The lookup tables the inputs of the model provide, by their names; a lookup whose result column has an array or
  /// object type returns the index of a constant object, which an expression may only assign to an object attribute
  std::unordered_map< std::string, const LookupTable*> lookupTables;
  /// The matrices and objects the inputs of the model provide, by their names, which expressions read as constant
  /// objects and never write
  std::unordered_map< std::string, const Input*> inputs;
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
  /// @brief Method returning the pointer holding the object of an object attribute in the status or data, through
  /// which the object can be replaced.
  std::shared_ptr<const Object>& getObject(const Attribute* object, Status& status, Data& data) const;
  /// @copydoc getObject(const Attribute*, Status&, Data&) const
  std::shared_ptr<const Object>& getObject(const Attribute* object, Status& status, SharedData& data) const;
  /**
   * @brief Method assigning an array or object of the given schema to an object attribute as a whole.
   *
   * The value is kept or converted for the target schema built from the effective schema of the attribute, which
   * the schema provider gives, and the layout of its current object: a fixed array keeps its size, a shorter value
   * being padded with undefined values and a longer one being cut off if only undefined values are cut and rejected
   * otherwise, and an open array takes the size of the value. A value whose schema the target schema supports is
   * kept, so that it is shared.
   */
  void setObject(const Attribute* object, Status& status, Data& data, const std::shared_ptr<const Object>& value, const Schema& valueSchema, const SchemaProvider& schemaProvider) const;
  /// @copydoc setObject(const Attribute*, Status&, Data&, const std::shared_ptr<const Object>&, const Schema&, const SchemaProvider&) const
  void setObject(const Attribute* object, Status& status, SharedData& data, const std::shared_ptr<const Object>& value, const Schema& valueSchema, const SchemaProvider& schemaProvider) const;
  /// @brief Method setting the value of an attribute and returning the change of the objective, which is the
  /// change of the value times the weight of the attribute for a data attribute and zero for a status attribute,
  /// the previous value being available here and nowhere later.
  BPMNOS::number setValue(const Attribute* attribute, Status& status, Data& data, std::optional<BPMNOS::number> value) const;
  /// @copydoc setValue(const Attribute*, Status&, Data&, std::optional<BPMNOS::number>) const
  BPMNOS::number setValue(const Attribute* attribute, Status& status, SharedData& data, std::optional<BPMNOS::number> value) const;
private:
  friend class Attribute;
  void add(Attribute* attribute);
  /// The number of attributes declared so far, which the registry of a scope continues from that of the scope
  /// enclosing it
  size_t declared = 0;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_AttributeRegistry_H
