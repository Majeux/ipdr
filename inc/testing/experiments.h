#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "cli-parse.h"
#include "pdr-context.h"
#include "pdr-model.h"
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
    std::vector<std::unique_ptr<IpdrResult>> results;
    double avg_time;
    double std_dev_time;

    Run(std::vector<std::unique_ptr<IpdrResult>> const& r);
    virtual ~Run() {}
    std::string str(::pdr::experiments::output_format fmt) const;
    std::string str_compared(
        const Run& other, ::pdr::experiments::output_format fmt) const;

   private:
    // latex export
    Table_t listing() const;
    Table_t combined_listing(const Run& other) const;
    Table_t summary() const;
  };

  class Experiment
  {
   public:
    Experiment(my::cli::ArgumentList const& a, Logger& l);
    virtual ~Experiment() {}
    void run();

   protected:
    my::cli::ArgumentList const& args;
    std::string model;
    std::string type;

    Tactic tactic;

    Logger& log;
    unsigned N_reps;
    std::vector<unsigned> seeds;

    tabulate::Table sample_table;
    tabulate::Table control_table;

    virtual std::unique_ptr<Run> single_run(bool is_control) = 0;
  };
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
