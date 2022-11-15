#ifndef PEBBLING_EXPERIMENTS_H
#define PEBBLING_EXPERIMENTS_H

#include "cli-parse.h"
#include "experiments.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-model.h"
#include "peterson.h"
#include "result.h"

#include <tabulate/table.hpp>
#include <variant>

namespace pdr::pebbling::experiments
{
  void pebbling_run(pebbling::PebblingModel& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);

  struct PebblingRun : public ::pdr::experiments::Run
  {
    using Row_t   = tabulate::Table::Row_t;
    using Table_t = std::array<Row_t, 7>;
    std::string model;
    Tactic tactic;

    double avg_time;
    double std_dev_time;
    std::optional<PebblingResult::PebblingInvariant> min_inv;
    std::optional<PebblingResult::PebblingTrace> min_strat;

    Run(const my::cli::ArgumentList& args,
        const std::vector<pebbling::PebblingResult>& r);
    std::string str(::pdr::experiments::output_format fmt) const;
    std::string str_compared(
        const Run& other, ::pdr::experiments::output_format fmt) const;

   private:
    // latex export
    Table_t table() const;
    Table_t combined_table(const Run& other) const;
  };

  class PebblingExperiment : pdr::experiments::Experiment
  {
    using superRun = ::pdr::experiments::Run;

   public:
    PebblingExperiment(
        my::cli::ArgumentList const& a, PebblingModel& m, Logger& l);

   private:
    PebblingModel& ts;
    my::cli::model_t::Peterson ts_descr;

    std::unique_ptr<superRun> single_run(bool is_control) override;
  };
} // namespace pdr::pebbling::experiments

#endif // EXPERIMENTS_H
