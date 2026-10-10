#ifndef BPMNOS_Model_Value_H
#define BPMNOS_Model_Value_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Number.h"

namespace BPMNOS {

enum ValueType { BOOLEAN, INTEGER, DECIMAL, STRING, COLLECTION };

typedef std::variant< bool, int, double, std::string > ValueVariant;

typedef std::optional<number> Value;

typedef std::unordered_map< std::string, Value > ValueMap;

/**
 * @brief A list of values that is neither status nor data, such as the header of a message.
 */
typedef std::vector<Value> Values;

/**
 * @brief An object, i.e. a structured value with arrays, held by a status or data and shared copy-on-write.
 */
struct Object;

/**
 * @brief The status of a token: its attribute values, indexed by `attribute->index`, and its objects, indexed
 * in their own numbering.
 */
struct Status {
  Status() = default;
  explicit Status(size_t size) : attributes(size) {}
  std::vector<Value> attributes;
  std::vector< std::shared_ptr<const Object> > objects;
};

struct SharedData;

/**
 * @brief Data: the attribute values, indexed by `attribute->index`, and the objects, indexed in their own
 * numbering, held by a state machine or copied from a data chain.
 */
struct Data {
  Data() = default;
  explicit Data(size_t size) : attributes(size) {}
  /// @brief Copies the values and objects a data chain refers to.
  Data(const SharedData& data);
  std::vector<Value> attributes;
  std::vector< std::shared_ptr<const Object> > objects;
};

/**
 * @brief A data chain: references to the attribute values and objects of the data of a scope and of the scopes
 * enclosing it, the outermost first.
 */
struct SharedData {
  SharedData() = default;
  /// @brief Extends a data chain by the data of a scope.
  SharedData(const SharedData& other, Data& data);
  /// @brief Begins a data chain with the data of a scope.
  SharedData(Data& data);
  /// @brief Appends the attribute values and objects of the data of a scope.
  void add(Data& data);
  std::vector< std::reference_wrapper<Value> > attributes;
  std::vector< std::reference_wrapper< std::shared_ptr<const Object> > > objects;
};

typedef std::unordered_map< std::string, std::variant< Value, std::string > > VariedValueMap;

/**
 * @brief Converts a string to a number.
 */
number to_number(const std::string& valueString, const ValueType& type);

/**
 * @brief Converts a value as it is written to a number.
 */
number to_number(const ValueVariant& value, const ValueType& type);

/**
 * @brief Converts the result of an evaluated expression to a value, keeping an absent result absent.
 *
 * An expression is evaluated at the precision of a double, whereas a value is a number, so a result that
 * becomes a value is converted here. The conversion loses precision and is therefore asked for rather than
 * done on the way out of the evaluation, so that a caller deriving further values from a result — the
 * discretizer of a choice being the one that does — keeps the precision it was evaluated at.
 */
Value to_value(std::optional<double> result);

/**
 * @brief Converts a number to a string.
 */
std::string to_string(number numberValue, const ValueType& type);

/**
 * @brief Returns the merge of the statuses of the tokens meeting at a join: the latest timestamp, and for every
 * other attribute the value the statuses agree on, which is undefined if two of them differ.
 **/
Status mergeStatus(const std::vector<Status>& statuses);

} // BPMNOS

#endif // BPMNOS_Model_Value_H
