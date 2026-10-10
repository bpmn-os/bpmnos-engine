#ifndef BPMNOS_Model_Schema_H
#define BPMNOS_Model_Schema_H

#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "model/utility/src/Value.h"

namespace BPMNOS::Model {

/**
 * @brief The shape of an attribute, as stated by its `type`.
 *
 * A type is a base followed by its dimensions, `type := base dimension*`. The base is a scalar type, one of
 * `boolean`, `integer`, `decimal`, `string` and `collection`, or an object `{ name: type, ... }` whose fields
 * have types of their own. A dimension is written `[n]` if its size is fixed and `[]` if it is open, and the
 * dimensions are read from left to right in the order of the indices: `boolean[3][4]` has 3 elements, each a
 * `boolean[4]`, accessed as `x[i][j]` with `i <= 3` and `j <= 4`. A schema without dimensions whose base is a
 * scalar type describes a scalar attribute; every other schema describes an object.
 */
struct Schema {
  std::optional<ValueType> scalar; ///< The scalar type of the base, or nullopt if the base is an object
  std::vector< std::pair<std::string, Schema> > fields; ///< The fields of an object base, in declaration order
  std::vector< std::optional<size_t> > dimensions; ///< The sizes of the dimensions in index order, nullopt if open

  /// @brief Parses the text of a type, throwing an error describing the first violation of the grammar.
  static Schema parse(const std::string& text);
  /// @brief Returns true if the schema describes a scalar attribute.
  bool isScalar() const { return scalar.has_value() && dimensions.empty(); }
  /// @brief Returns the text of the type in its canonical form.
  std::string stringify() const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Schema_H
