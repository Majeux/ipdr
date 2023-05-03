#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "cli-parse.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"

#include <optional>
#include <tabulate/exporter.hpp>
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

  std::string time_str(double x);

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
    std::string model;
    std::string tactic;
    double avg_time;
    double std_dev_time;
    std::optional<double> avg_inc_time;
    std::optional<double> std_dev_inc_time;

    Run(std::string const& m, std::string const& t,
        std::vector<std::unique_ptr<IpdrResult>>&& r);
    Run(Run const& r) = delete; // cannot copy vector<unique_ptr>
    virtual ~Run() {}

    std::string str(::pdr::experiments::output_format fmt) const;
    std::string str_compared(
        const Run& other, ::pdr::experiments::output_format fmt) const;
    void dump(tabulate::Exporter& exp, std::ostream& out) const;

   protected:
    Row_t tactic_row() const;
    Row_t avg_time_row() const;
    Row_t std_time_row() const;
    Row_t avg_inc_time_row() const;
    Row_t std_inc_time_row() const;
    // latex export, called by str and str_compared to show results
    virtual tabulate::Table make_table() const;
    // must pass a Run of same subtype
    virtual tabulate::Table make_combined_table(const Run& other) const;
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

    // reset the sample and control tables to the header
    virtual void reset_tables() = 0;

    virtual std::shared_ptr<Run> do_reps(const bool is_control) = 0;
  };
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
