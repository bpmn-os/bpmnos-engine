#ifndef BPMNOS_Model_Value_H
#define BPMNOS_Model_Value_H

#include <functional>
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

struct SharedValues;

struct Values : std::vector<Value> {
  Values() = default;
  Values(size_t size) : std::vector<Value>(size) {}
  Values(std::initializer_list<Value> init) : std::vector<Value>(init) {}
  Values(const SharedValues& values);
};

struct SharedValues : std::vector< std::reference_wrapper< Value > > {
  SharedValues() = default;
  SharedValues(const SharedValues& other,Values& values);
  SharedValues(Values& values);
  void add(Values& values);
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
 * Returns merged values from a set of values
 **/
Values mergeValues(const std::vector<Values>& valueSets);

} // BPMNOS

#endif // BPMNOS_Model_Value_H
