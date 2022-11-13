#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "cli-parse.h"
#include "result.h"

#include <tabulate/format.hpp>
#include <tabulate/table.hpp>

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

  using Row_t   = tabulate::Table::Row_t;
  using Table_t = std::array<Row_t, 7>;

  class Run
  {
   public:
    double avg_time;
    double std_dev_time;

    std::string str(::pdr::experiments::output_format fmt) const;
    std::string str_compared(
        const Run& other, ::pdr::experiments::output_format fmt) const;

   private:
    // latex export
    Table_t listing() const;
    Table_t combined_listing(const Run& other) const;
  };

  class Experiment
  {
   public:
    my::cli::ArgumentList const& args;
    std::string model;
    Tactic tactic;

    tabulate::Table sample_table;
    tabulate::Table control_table;
    std::vector<std::unique_ptr<IpdrResult>> reps;
    std::vector<std::unique_ptr<IpdrResult>> control_reps;

    Experiment(my::cli::ArgumentList const& a);

   private:
    unsigned N_reps;

    virtual Run do_run(bool control_run) = 0;
  };
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
