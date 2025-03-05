#ifndef BPMNOS_Number_H
#define BPMNOS_Number_H

#include <cnl/all.h>
#include <limits>
#include <string>
#include <vector>
#include <optional>
#include <functional>

#include "Value.h"

//#define BPMNOS_NUMBER_TYPE double

//scaled_integer< int32_t, power<-8> > has max: 8.4 million, and  precision: ~ 0.004
//scaled_integer< int64_t, power<-16> > has max: 1.4e14, and precision: ~ 0.000015

#ifndef BPMNOS_NUMBER_TYPE
#define BPMNOS_NUMBER_REP int64_t
#define BPMNOS_NUMBER_SCALE 16
#define BPMNOS_NUMBER_TYPE cnl::scaled_integer< BPMNOS_NUMBER_REP, cnl::power<-BPMNOS_NUMBER_SCALE> >
// Specialize std::hash for BPMNOS_NUMBER_TYPE
namespace std {
  template <>
  struct hash<BPMNOS_NUMBER_TYPE> {
    std::size_t operator()(const BPMNOS_NUMBER_TYPE& value) const {
      // Hash the underlying value
      return std::hash<BPMNOS_NUMBER_REP>()(cnl::unwrap(value));
    }
  };

  template <>
  struct hash<const BPMNOS_NUMBER_TYPE> {
    std::size_t operator()(const BPMNOS_NUMBER_TYPE& value) const {
      // Hash the underlying value
      return std::hash<BPMNOS_NUMBER_REP>()(cnl::unwrap(value));
    }
  };
}
#define BPMNOS_NUMBER_PRECISION (1.0 / (1 << BPMNOS_NUMBER_SCALE))
#endif

#ifndef BPMNOS_NUMBER_PRECISION
#define BPMNOS_NUMBER_PRECISION 1e-6
#endif


namespace BPMNOS {

  typedef BPMNOS_NUMBER_TYPE number;
  typedef std::unordered_map< std::string, std::optional<number> > ValueMap;

  struct SharedValues;

  struct Values : std::vector<std::optional<number>> {
    Values() = default;
    Values(const SharedValues& values);
  };

  struct SharedValues : std::vector< std::reference_wrapper< std::optional<number> > > {
    SharedValues() = default;
    SharedValues(const SharedValues& other,Values& values);
    SharedValues(Values& values);
    void add(Values& values);
  };
  
  typedef std::unordered_map< std::string, std::variant< std::optional<number>, std::string > > VariedValueMap;

  double stod(const std::string& str);
  int stoi(const std::string& str);

  /**
   * @brief Converts a string to a number.
   */
  number to_number(const std::string& valueString, const ValueType& type);

  /**
   * @brief Converts a value to a number.
   */
  number to_number(const Value& value, const ValueType& type);

  /**
   * @brief Converts a number to a string.
   */
  std::string to_string(number numberValue, const ValueType& type);

  /**
   * @brief Converts a double to a string without trailing zeros after the decimal point.
   */
  std::string to_string(double value);

  /**
   * Returns merged values from a set of values
   **/
  BPMNOS::Values mergeValues(const std::vector<BPMNOS::Values>& valueSets);

} // namespace BPMNOS

#endif // BPMNOS_Number_H

