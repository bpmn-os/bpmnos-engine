#ifndef BPMNOS_Model_Schema_H
#define BPMNOS_Model_Schema_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "model/utility/src/Value.h"
#include "model/utility/src/Object.h"

namespace BPMNOS::Model {

/**
 * @brief The shape of an attribute, as stated by its `type`.
 *
 * A type is a base followed by its dimensions, `type := base dimension*`. The base is a scalar type, one of
 * `boolean`, `integer`, `decimal` and `string`, or an object `{ name: type, ... }` whose fields
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

  /**
   * @brief Fixes the sizes a size declaration states, given as the text following the name of the attribute.
   *
   * The text is a path of dimensions and fields, e.g. `[6].flags[3]`, in which every number in brackets is the
   * size of the next dimension of the base or field it follows and `[]` leaves that dimension as it is. A size
   * contradicting a size fixed before, more dimensions than the type has, and an unknown field are errors.
   */
  void declareSizes(const std::string& steps);

  /// @brief Returns true if every dimension, also of the fields, has a fixed size.
  bool isFixed() const;

  /**
   * @brief Returns an object of this schema with every value undefined, an open dimension having no elements.
   */
  std::shared_ptr<const BPMNOS::Object> undefinedObject() const;

  /**
   * @brief Returns an object of this schema holding the values of the given constant object.
   *
   * The constant object must have as many dimensions as the schema at the base and at every field, its
   * scalar values must be convertible to the types of the schema, and its fields must be fields of the schema.
   * A dimension of fixed size is padded with undefined values and must not be exceeded, an open dimension takes
   * the length of the constant object, and a field it lacks is undefined. If the constant object has exactly the
   * layout of the schema, it is returned itself, so that it is shared.
   */
  std::shared_ptr<const BPMNOS::Object> conform(const std::shared_ptr<const BPMNOS::Object>& constant) const;

  /**
   * @brief Returns the schema an object of the given layout imposes on a value assigned to it.
   *
   * A value assigned to the whole object keeps the sizes of the fixed dimensions and takes those of the open
   * ones. A value assigned to an element keeps every size, except a size zero of an open dimension, which the
   * value determines.
   */
  static Schema of(const BPMNOS::Object::Layout& layout, bool element);

  /**
   * @brief Returns an object of this schema holding the values of the given JSON text.
   *
   * An array is a dimension, whose elements have equal lengths at an open dimension and at most the declared
   * length at a fixed one, an object is a value with fields, whose keys must be fields of the schema, and a number,
   * truth value or string is a value of the type the schema states, `null` being undefined. A fixed dimension is
   * padded with undefined values, an open dimension takes the length of the JSON array, and a field the JSON lacks
   * is undefined.
   */
  std::shared_ptr<const BPMNOS::Object> fromJSON(const std::string& text) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Schema_H
