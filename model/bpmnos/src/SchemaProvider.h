#ifndef BPMNOS_Model_SchemaProvider_H
#define BPMNOS_Model_SchemaProvider_H

#include "model/utility/src/Schema.h"

namespace BPMNOS::Model {

class Attribute;

/**
 * @brief Interface providing the effective schema of an object attribute, which results from its attribute type by the
 * size declarations of the instance data.
 *
 * The functions writing objects belong to the model, which does not know where the effective schemas are held;
 * they obtain them through this interface.
 */
class SchemaProvider {
public:
  virtual ~SchemaProvider() = default;
  /// @brief Returns the effective schema of an object attribute.
  virtual const BPMNOS::Schema& getSchema(const Attribute* object) const = 0;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SchemaProvider_H
