#ifndef BPMNOS_Model_Input_H
#define BPMNOS_Model_Input_H

#include <memory>
#include <string>
#include "model/utility/src/Object.h"
#include "model/bpmnos/src/extensionElements/Schema.h"

namespace BPMNOS::Model {

/**
 * @brief An input a data store declares: data that is given once for a run and only read.
 *
 * A `lookup` is a CSV file with a header, whose `schema` states every column with its type, and becomes a
 * function of its name (see @ref LookupTable). A `matrix` is a CSV file without a header, two-dimensional with
 * equally long rows, whose cells are values or literals of the base of its schema. An `object` is a JSON file.
 * A matrix and an object are read and fitted to their schema when the model is loaded, padded at fixed dimensions,
 * and are read by their name in every expression, `distance[i][j]`, as a constant object that no expression
 * writes.
 */
class Input {
public:
  enum class Type { LOOKUP, MATRIX, OBJECT };
  /// @brief Constructs an input from the content of its source.
  Input(std::string name, Type type, Schema schema, const std::string& content);
  const std::string name;
  const Type type;
  const Schema schema; ///< The schema of a matrix or an object; for a lookup, the type of its result column
  /// The constant object a matrix or an object holds, nullptr for a lookup
  std::shared_ptr<const BPMNOS::Object> object;
private:
  std::shared_ptr<const BPMNOS::Object> readMatrix(const std::string& content) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Input_H
