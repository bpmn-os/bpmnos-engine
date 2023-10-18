#ifndef BPMNOS_Model_Lookup_H
#define BPMNOS_Model_Lookup_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Operator.h"
#include "LookupTable.h"

namespace BPMNOS::Model {

class Lookup : public Operator {
public:
  Lookup(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
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
 * @brief Applies the lookup operator to update a status attribute.
 *
 * This function update a status attribute by querying a table with specified
 * lookup attributes. It then updates a status attribute based on the lookup result.
 * The function performs the following steps:
 *
 * - For each attribute to be looked up, it checks if the corresponding status
 *   value is available. If not, it sets the target attribute to undefined and
 *   returns.
 * - It performs a lookup in a table using the specified lookup attributes.
 * - If a lookup result is available, it updates the target attribute in the
 *   status with the lookup value.
 * - If no lookup result is available, it sets the target attribute in the status
 *   to undefined (std::nullopt).
 *
 * @param status The status values to be updated.
 */
  void apply(Values& status) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Lookup_H
