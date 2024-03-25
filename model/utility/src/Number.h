#ifndef BPMNOS_Model_Number_H
#define BPMNOS_Model_Number_H

#include <cnl/all.h>
#include <limits>
#include <string>
#include <vector>
#include <optional>
#include "Value.h"
#include <functional>

//#define BPMNOS_NUMBER_TYPE double

//scaled_integer< int32_t, power<-8> > has max: 8.4 million, and  precision: ~ 0.004
//scaled_integer< int64_t, power<-16> > has max: 1.4e14, and precision: ~ 0.000015

#ifndef BPMNOS_NUMBER_TYPE
  #define BPMNOS_NUMBER_TYPE cnl::scaled_integer< int64_t, cnl::power<-16> >
  #define BPMNOS_NUMBER_HASH std::hash<int64_t>()(cnl::unwrap(value))
  #define BPMNOS_NUMBER_PRECISION (1.0 / (1 << 16))
#endif

#ifndef BPMNOS_NUMBER_HASH
  #define BPMNOS_NUMBER_HASH std::hash<BPMNOS_NUMBER_TYPE>()(value)
  #define BPMNOS_NUMBER_PRECISION 1e-6
#endif

namespace BPMNOS {

  typedef BPMNOS_NUMBER_TYPE number;
  typedef std::unordered_map< std::string, std::optional<number> > ValueMap;
  typedef std::vector< std::optional<number> > Values;
  struct Globals : std::vector< std::reference_wrapper< std::optional<number> > > {
    Globals() = default;
    Globals(const Globals& other,Values& values);
    Globals(Values& values);
    void add(Values& values);
  };
  
  typedef std::unordered_map< std::string, std::variant< std::optional<number>, std::string > > VariedValueMap;

  struct ValueHash {
    size_t operator()(const number& value) const {
        return BPMNOS_NUMBER_HASH;
    }
  };

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
   * Returns merged values from a set of values
   **/
  BPMNOS::Values mergeValues(const std::vector<BPMNOS::Values>& valueSets);

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Number_H

