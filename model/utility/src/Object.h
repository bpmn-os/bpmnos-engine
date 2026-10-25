#ifndef BPMNOS_Model_Object_H
#define BPMNOS_Model_Object_H

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "Value.h"
#include <nlohmann/json.hpp>

namespace BPMNOS {

struct Schema;
struct Layout;

/**
 * @brief The layout of an array: the layout of its elements, which all elements share, and the number of elements.
 */
struct ArrayLayout {
  std::shared_ptr<const Layout> layout; ///< The element layout
  size_t size; ///< The number of elements
};

/**
 * @brief The layout of a field of an object: the position of its first value relative to the first value of the
 * object, and its layout.
 */
struct FieldLayout {
  size_t offset;
  std::shared_ptr<const Layout> layout;
};

/**
 * @brief The layout of an object: the layouts of its fields, in the order of the fields of its schema.
 */
struct ObjectLayout {
  std::vector<FieldLayout> fields;
};

/**
 * @brief The geometry of an object: a single value, an array or an object, the schema it lays out, and the number of
 * positions it occupies in the values of the object.
 *
 * A layout and its schema agree: the layout has the structure of the schema, the schema states the value type of
 * every single value and the name of every field, and every array the schema fixes has the size the schema states.
 * The layout of an element or field refers to the corresponding part of the schema. A layout never changes once
 * built, and refers to the layouts it contains through `std::shared_ptr<const Layout>`, so that a layout derived from
 * another shares every part it does not change.
 */
struct Layout : std::variant<std::monostate, ArrayLayout, ObjectLayout> {
  std::shared_ptr<const Schema> schema; ///< The schema of the object
  size_t size; ///< The number of positions the layout occupies

  /// @brief Constructs the layout of a single value of the given schema, a value type.
  explicit Layout(std::shared_ptr<const Schema> schema);
  /// @brief Constructs the layout of an array of the given schema, an array type, with the given element layout and
  /// number of elements.
  Layout(std::shared_ptr<const Schema> schema, std::shared_ptr<const Layout> layout, size_t size);
  /// @brief Constructs the layout of an object of the given schema, an object type, with the given layouts of its
  /// fields, the offsets following from their sizes.
  Layout(std::shared_ptr<const Schema> schema, const std::vector< std::shared_ptr<const Layout> >& fields);

  /// @brief Returns the schema in its notation with every array at its actual size.
  std::string stringify() const;
  /// @brief Returns true if the layouts have the same structure, sizes and offsets and their schemas are equal.
  bool operator==(const Layout& other) const;
};

/**
 * @brief An object, i.e. an array or a structured value, stored flat.
 *
 * The values of an object are held in one vector, and its layout states where each of them is and, through its
 * schema, what value type it has. An object is shared copy-on-write wherever it is held, as a
 * `std::shared_ptr<const Object>`, so that copying a status or data copies no values.
 */
struct Object {
  std::shared_ptr<const Layout> layout;
  std::vector<Value> values;

  Object() = default;
  /**
   * @brief Constructs an object of the given schema with every value undefined, every open array having size 0.
   */
  explicit Object(const Schema& schema);
  /**
   * @brief Constructs an object of the given schema holding the values of the given JSON value.
   *
   * A JSON array is an array, whose elements have equal sizes at an open array and at most the declared size at a
   * fixed one, a JSON object is an object, whose keys must be fields of the schema, and a number, truth value or
   * text is a single value of the value type the schema states, `null` being undefined. A fixed array is padded with
   * undefined values, an open array takes the size of the JSON array, and a field the JSON lacks is undefined.
   */
  Object(const Schema& schema, const nlohmann::json& json);
  /**
   * @brief Constructs an object of the given schema, the target, from the given source object, whose schema is the
   * schema of its layout.
   *
   * At a value type, the source holds a single value of the same value type; arrays and objects are not cast. At an
   * array type, the source is an array. A fixed array of size n has size n, a shorter source being padded with
   * undefined elements and a longer source being cut off if every value beyond the n-th element is undefined and
   * rejected otherwise; an open array has the size of the source. At an object type, the source is an object whose
   * fields are fields of the target; a field the source lacks is undefined, every open array in it having size 0.
   * If the layout determined equals the layout of the source, the object takes the layout of the source. A source
   * that cannot be converted is rejected with an error.
   */
  Object(const Schema& schema, const Object& source);
};

/**
 * @brief Renders an object as a literal, `[ ... ]` for an array and `{ name := value, ... }` for a value with
 * fields, strings quoted, decimals written with a decimal point, and undefined values written `undefined`.
 */
std::string to_string(const Object& object);

/**
 * @brief Renders an object as JSON: an array as an array, fields as an object keyed by their names in their order, a
 * truth value as a truth value, an integer or decimal as a number of its type, a string as text, and an undefined
 * value as `null`.
 */
nlohmann::ordered_json to_json(const Object& object);

/**
 * @brief Returns the object a slot holds for modification, replacing it by a copy first if anyone else holds it,
 * so that a modification is seen by the slot alone.
 *
 * Every object held by more than one status or data, and every constant object, is copied once when it is first
 * written, whereas an object held by the slot alone is modified in place.
 */
Object& modifiable(std::shared_ptr<const Object>& slot);

/**
 * @brief Returns a copy of an object in which an array has the given size.
 *
 * The array is reached from the object by the given steps, each the name of a field or `std::nullopt` for the
 * element of an array, as the steps of a path are without their indices. All elements of an array having the same
 * layout, an array within the elements of another changes in every element. Elements beyond the new size are
 * dropped, and elements added are undefined. An array fixed in the schema of the object stays fixed at the new
 * size.
 */
std::shared_ptr<Object> resize(const Object& object, const std::vector< std::optional<std::string> >& steps, size_t size);

/**
 * @brief Returns the merge of objects of the same attribute, held by the statuses merged at a join.
 *
 * Every array takes the largest size the objects have, and is fixed only where all objects fix it at the same size.
 * Each value is merged from the objects holding it, an object whose size an array exceeds holding no value there:
 * values that agree are kept, an undefined value is no conflict, and a conflict makes the value undefined. Objects
 * that are all the same are returned as they are.
 */
std::shared_ptr<const Object> merge(const std::vector< std::shared_ptr<const Object> >& objects);

/**
 * @brief Returns the values of an array of single values as decimals, an undefined value as a quiet NaN, throwing an
 * error for any other object.
 */
std::vector<double> to_vector(const Object& object);

} // namespace BPMNOS

#endif // BPMNOS_Model_Object_H
