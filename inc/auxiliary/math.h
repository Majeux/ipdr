#ifndef MYMATH_H
#define MYMATH_H

#include <cmath>
#include <numeric> // std::accumulate
#include <string>
#include <type_traits>
#include <vector>

namespace my::math
{
  using std::vector;

  template <typename NumericT>
  double percentage_dec(NumericT old_v, NumericT new_v)
  {
    static_assert(
        std::is_arithmetic<NumericT>::value, "NumericT is not a numeric type.");

    double a = old_v;
    double b = new_v;
    return (double)(a - b) / a * 100;
  }

  template <typename NumericT>
  double percentage_inc(NumericT old_v, NumericT new_v)
  {
    static_assert(
        std::is_arithmetic<NumericT>::value, "NumericT is not a numeric type.");

    double a = old_v;
    double b = new_v;
    return (b - a) / a * 100;
  }

  template <typename NumericT>
  double mean(vector<NumericT> const& values)
  {
    return std::accumulate(values.cbegin(), values.cend(), 0) / values.size();
  }

  // compute standard deviation when the mean average is known
  template <typename NumericT>
  double std_dev(const vector<NumericT>& values, double mean)
  {
    static_assert(
        std::is_arithmetic<NumericT>::value, "NumericT is not a numeric type.");

    double total_variance = std::accumulate(values.cbegin(), values.cend(), 0.0,
        [mean](NumericT a, NumericT t) { return a + (t - mean) * (t - mean); });

    return std::sqrt(total_variance / values.size());
  }

  // compute standard deviation
  template <typename NumericT> double std_dev(const vector<NumericT>& values)
  {
    static_assert(
        std::is_arithmetic<NumericT>::value, "NumericT is not a numeric type.");

    return std_dev(values, mean(values));
  }

} // namespace my::math

#endif // MYMATH_H
