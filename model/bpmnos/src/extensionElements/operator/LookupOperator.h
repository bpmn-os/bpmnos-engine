#ifndef BPMNOS_Model_LookupOperator_H
#define BPMNOS_Model_LookupOperator_H

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/Operator.h"
#include "LookupTable.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator that uses a lookup table to determine that value
 * of a status attribute.
 **/
class LookupOperator : public Operator {
public:
  LookupOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);
  std::string filename;
  std::string key;
  std::vector< std::pair< std::string, Attribute*> > lookups;

  LookupTable* table;

  static inline std::unordered_map< std::string, LookupTable > lookupTable = {};
  static inline LookupTable* getLookupTable(const std::string& filename) {
    if ( lookupTable.find(filename) == lookupTable.end() ) {
      lookupTable[filename] = LookupTable(filename);
    }
    return &lookupTable[filename];
  };

/**
 * @brief Applies the lookup operator to update an attribute value.
 *
 * This function update a status attribute by querying a table with specified
 * lookup attributes. It then updates an attribute value based on the lookup result.
 * The function performs the following steps:
 *
 * - For each attribute to be looked up, it checks if the corresponding attribute
 *   value is available. If not, it sets the target attribute to undefined and
 *   returns.
 * - It performs a lookup in a table using the specified lookup attributes.
 * - If a lookup result is available, it updates the target attribute with the lookup value.
 * - If no lookup result is available, it sets the target attribute to undefined (std::nullopt).
 */
  template <typename DataType>
  void _apply(BPMNOS::Values& status, DataType& data) const;

  void apply(BPMNOS::Values& status, BPMNOS::Values& data) const override { return _apply(status,data); };
  void apply(BPMNOS::Values& status, BPMNOS::SharedValues& data) const override { return _apply(status,data); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_LookupOperator_H
