#ifndef BPMNOS_Numeric_H
#define BPMNOS_Numeric_H

#include <limits>

namespace BPMNOS {

  /**
   * @brief Precision used for numeric comparisons.
   */
  template <typename T>
  constexpr double numeric_precision = double(std::numeric_limits< T >::epsilon());

  /**
   * @brief Half precision used for rounding to the numeric type.
   */
  template <typename T>
  constexpr double numeric_correction = double(std::numeric_limits< T >::epsilon())/2 - std::numeric_limits<double>::epsilon();


  /**
   * @brief Converts a long unsigned integer value to numeric type T.
   */
/*
  template <typename T>
  T numeric(const long unsigned int& value) { return value; }
*/

  /**
   * @brief Converts a long integer value to numeric type T.
   */
/*
  template <typename T>
  T numeric(const long int& value) { return value; }
*/

  /**
   * @brief Converts an unsigned integer value to numeric type T.
   */
/*
  template <typename T>
  T numeric(const unsigned int& value) { return value; }
*/

  /**
   * @brief Converts an integer value to numeric type T.
   */
/*
  template <typename T>
  T numeric(const int& value) { return value; }
*/

  /**
   * @brief Converts a double value roundig it to the nearest number of numeric type T.
   */
  template <typename T>
  T numeric(const double& value) {
    return value > 0 ? value + numeric_correction<T> : value - numeric_correction<T>;
  }

} // namespace BPMNOS

#endif // BPMNOS_Numeric_H

