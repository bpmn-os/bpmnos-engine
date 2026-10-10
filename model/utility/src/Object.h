#ifndef BPMNOS_Model_Object_H
#define BPMNOS_Model_Object_H

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Value.h"

namespace BPMNOS {

/**
 * @brief An object, i.e. an array or a structured value, stored flat.
 *
 * The values of an object are held in one vector, and its layout states where each of them is: an element is
 * one slot at an offset computed from the layout. An object is shared copy-on-write wherever it is held, as a
 * `std::shared_ptr<const Object>`, so that copying a status or data copies no values.
 */
struct Object {
  /**
   * @brief The shape of an object with the sizes of its dimensions known.
   *
   * A layout is a base followed by its dimensions, read from left to right in the order of the indices. The
   * base is a scalar type or a list of fields, each with a layout of its own, stored one after the other within
   * an element. The elements of an array are uniform, so that they share their layout and are a stride apart:
   * the element at the indices `i_1, ..., i_k`, counting from zero, begins at
   * `((i_1 * d_2 + i_2) * d_3 + ... + i_k) * stride`.
   */
  struct Layout {
    struct Field;
    std::optional<ValueType> scalar; ///< The scalar type of the base, or nullopt if the base has fields
    std::vector<Field> fields; ///< The fields of the base, in declaration order
    std::vector<size_t> dimensions; ///< The sizes of the dimensions, in index order
    size_t stride = 1; ///< The number of values of one element of the base
    /// For each dimension whether its size is fixed, by the type or a size declaration, so that an assignment
    /// pads a shorter value and rejects a longer one, whereas an open dimension takes the length of the value;
    /// empty for a constant object
    std::vector<bool> fixed;

    /// @brief Returns the number of values of an object of this layout.
    size_t size() const;
    /// @brief Returns the type the layout states, in the syntax of attribute types.
    std::string stringify() const;
    /// @brief Recomputes the offsets of the fields and the stride from the layouts of the fields.
    void arrange();
    bool operator==(const Layout& other) const;
  };

  std::shared_ptr<const Layout> layout;
  std::vector<Value> values;

  /// @brief Returns true if the object is an array of scalar values with a single dimension.
  bool isVector() const { return layout->scalar.has_value() && layout->dimensions.size() == 1; }
};

struct Object::Layout::Field {
  std::string name;
  size_t offset; ///< The position of the field's first value within an element of the base
  Layout layout;
  bool operator==(const Field& other) const = default;
};

/**
 * @brief Renders an object as a literal, `[ ... ]` for an array and `{ name := value, ... }` for a value with
 * fields, strings quoted and undefined values written `undefined`.
 */
std::string to_string(const Object& object);

/**
 * @brief Returns the object a slot holds for modification, replacing it by a copy first if anyone else holds it,
 * so that a modification is seen by the slot alone.
 *
 * Every object held by more than one status or data, and every constant object, is copied once when it is first
 * written, whereas an object held by the slot alone is modified in place.
 */
Object& modifiable(std::shared_ptr<const Object>& slot);

/**
 * @brief Returns a copy of an object in which a dimension has the given length.
 *
 * The dimension is the dimension with the given position of the base, if no fields are given, or of the field
 * the given fields name one within the other. All elements of an array being uniform, the dimension of a field
 * changes in every element. Elements beyond the new length are dropped, and elements added are undefined. The
 * dimension keeps its flag.
 */
std::shared_ptr<Object> resized(const Object& object, const std::vector<std::string>& fields, size_t dimension, size_t length);

/**
 * @brief Returns the merge of objects of the same schema, held by the statuses merged at a join.
 *
 * Every dimension takes the largest length the objects have, and each value is merged from the objects
 * holding it, an object whose length a dimension exceeds holding no value there: values that agree are kept,
 * an undefined value is no conflict, and a conflict makes the value undefined. Objects that are all the same
 * are returned as they are.
 */
std::shared_ptr<const Object> merge(const std::vector< std::shared_ptr<const Object> >& objects);

/**
 * @brief Returns the values of an array of scalar values with a single dimension as decimals, an undefined
 * value as a quiet NaN, throwing an error for any other object.
 */
std::vector<double> to_vector(const Object& object);

} // namespace BPMNOS

#endif // BPMNOS_Model_Object_H
