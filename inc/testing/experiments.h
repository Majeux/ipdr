#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "cli-parse.h"

#include <tabulate/table.hpp>
#include <tabulate/format.hpp>

namespace pdr::experiments
{
  enum output_format
  {
    string,
    latex,
    markdown
  };

  namespace math 
  {
    std::string time_str(double x);

    template <typename T> double percentage_dec(T old_v, T new_v)
    {
      double a = old_v;
      double b = new_v;
      return (double)(a - b) / a * 100;
    }

    template <typename T> double percentage_inc(T old_v, T new_v)
    {
      double a = old_v;
      double b = new_v;
      return (b - a) / a * 100;
    }

    // compute standard deviation
    double std_dev(const std::vector<double>& v);
    // compute standard deviation when the mean average is known
    double std_dev(const std::vector<double>& v, double mean);
  } // namespace math

  namespace tablef 
  {
    tabulate::Format& format_base(tabulate::Format& f);
    tabulate::Format& format_base(tabulate::Table& t);
    tabulate::Table init_table();
  } // namespace tablef
} // namespace pdr::pebbling::experiments

#endif // EXPERIMENTS_H
