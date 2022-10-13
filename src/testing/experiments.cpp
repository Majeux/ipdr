#include "experiments.h"

#include <fmt/format.h>
#include <numeric> // std::accumulate
#include <tabulate/format.hpp>
#include <tabulate/table.hpp>

namespace pdr::experiments
{
  namespace math
  {
    using std::vector;

    std::string time_str(double x) { return fmt::format("{:.3f} s", x); };

    double std_dev(const vector<double>& v)
    {
      double mean = std::accumulate(v.cbegin(), v.cend(), 0.0) / v.size();
      return std_dev(v, mean);
    }

    double std_dev(const vector<double>& v, double mean)
    {
      double total_variance = std::accumulate(v.begin(), v.end(), 0.0,
          [mean](double a, double t) { return a + (t - mean) * (t - mean); });

      return std::sqrt(total_variance / v.size());
    }
  } // namespace math

  namespace tablef
  {
    tabulate::Format& format_base(tabulate::Format& f)
    {
      return f.font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }

    tabulate::Format& format_base(tabulate::Table& t)
    {
      return format_base(t.format());
    }

    tabulate::Table init_table()
    {
      tabulate::Table t;
      format_base(t);
      return t;
    }
  } // namespace tablef

} // namespace pdr::experiments
