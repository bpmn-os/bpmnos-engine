#ifndef BPMNOS_Schema_H
#define BPMNOS_Schema_H

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "Value.h"
#include "Object.h"

namespace BPMNOS {

struct Schema;

/**
 * @brief An array type: the type of its elements and its size, which is std::nullopt if the array is open.
 */
struct ArrayType {
  std::shared_ptr<const Schema> type; ///< The element type
  std::optional<size_t> size; ///< The size of a fixed array, std::nullopt for an open array
};

/**
 * @brief A field of an object type: its name and its type.
 */
struct Field {
  std::string name;
  std::shared_ptr<const Schema> type;
};

/**
 * @brief An object type: its fields in the order of their declaration.
 */
struct ObjectType {
  std::vector<Field> fields;
};

/**
 * @brief A type: a value type, an array type or an object type.
 *
 * A schema is written in the notation `schema := ( valuetype | "{" field ( "," field )* "}" ) dimension*`, with
 * `field := name ":" schema` and `dimension := "[" n "]" | "[]"`, a dimension `[n]` denoting a fixed array of size
 * `n` and `[]` an open array. The dimensions are written outermost first: `boolean[3][4]` is the array type of size
 * 3 whose element type is the array type of size 4 whose element type is `boolean`.
 *
 * A schema never changes once built, and refers to the schemas it contains through `std::shared_ptr<const Schema>`,
 * so that a schema derived from another shares every part it does not change.
 */
struct Schema : std::variant<ValueType, ArrayType, ObjectType> {
  using variant::variant;
  Schema() = default;

  /**
   * @brief Constructs the target schema of an assignment to an object of the given current layout, whose effective
   * schema is given.
   *
   * The schema has the structure of the effective schema. Every array that is fixed in the effective schema is fixed
   * at the size it has in the current layout, and every open array stays open, so that an assignment keeps the size
   * of a fixed array, which may have been resized during the run, and gives an open array the size of the source.
   */
  Schema(const Schema& effectiveSchema, const BPMNOS::Layout& currentLayout);

  /// @brief Parses the notation of a type, throwing an error describing the first violation of the grammar.
  static Schema parse(const std::string& text);
  /// @brief Returns the notation of the type in its canonical form.
  std::string stringify() const;
  /// @brief Returns true if the schemas describe the same type: the same value types, the same arrays with the same
  /// sizes, and the same fields, in the same order and with equal types.
  bool operator==(const Schema& other) const;
  /**
   * @brief Returns true if every object of the given source schema is, unchanged, an object of this schema.
   *
   * The two schemas must have the same structure, the same value types and the same fields in the same order, and at
   * every array type this array must be open or both arrays must be fixed at the same size.
   */
  bool supports(const Schema& source) const;

  /**
   * @brief Returns the schema with the sizes a size declaration states, given as the text following the name of the
   * attribute.
   *
   * The text is a path of steps, e.g. `[6].flags[3]`. A step `[n]` states the size `n` for the next array, a step
   * `[]` states no size for it, and a step `.name` descends into the field `name`. The returned schema has new parts
   * along the path and shares every other part with this schema. A size of 0, a size contradicting one fixed before,
   * an index step where the schema has no array, and an unknown field are errors.
   */
  Schema declareSizes(const std::string& declaration) const;
};

} // namespace BPMNOS

#endif // BPMNOS_Schema_H
