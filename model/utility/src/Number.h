#ifndef BPMNOS_Number_H
#define BPMNOS_Number_H

#include <cnl/scaled_integer.h>
#include <limits>
#include <string>
#include <type_traits>

//#define BPMNOS_NUMBER_TYPE double

//scaled_integer< int32_t, power<-3,10> > has max: 2.1 million, and precision: 0.001
//scaled_integer< int64_t, power<-6,10> > has max: 9.2 trillion, and precision: 0.000001    
         
#define BPMNOS_NUMBER_REP int64_t
#define BPMNOS_NUMBER_SCALE 6
#define BPMNOS_NUMBER_TYPE cnl::scaled_integer< BPMNOS_NUMBER_REP, cnl::power<-BPMNOS_NUMBER_SCALE,10> >
#define BPMNOS_NUMBER_PRECISION 1e-6


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

// Ensure that a number multiplied by a double computes as floating point (fixed-point numbers could overflow) 
static_assert(
  std::is_floating_point_v<decltype(BPMNOS_NUMBER_TYPE{} * double{})>,
  "a number multiplied by a double must be computed in floating point"
);
static_assert(
  std::is_floating_point_v<decltype(double{} * BPMNOS_NUMBER_TYPE{})>,
  "a double multiplied by a number must be computed in floating point"
);


namespace BPMNOS {

  typedef BPMNOS_NUMBER_TYPE number;

  double stod(const std::string& str);
  int stoi(const std::string& str);

  /**
   * @brief Converts a double to a string without trailing zeros after the decimal point.
   */
  std::string to_string(double value);

} // namespace BPMNOS

#endif // BPMNOS_Number_H

