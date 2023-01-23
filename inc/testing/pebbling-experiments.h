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
  namespace expsuper = ::pdr::experiments;

  struct PebblingRun final : public ::pdr::experiments::Run
  {
    using Row_t   = tabulate::Table::Row_t;
    // 7 rows

    std::optional<PebblingResult::PebblingInvariant> min_inv;
    std::optional<PebblingResult::PebblingTrace> min_strat;
    PebblingRun(std::string const& m, std::string const& t,
        std::vector<std::unique_ptr<IpdrResult>>&& results);

   private:
    tabulate::Table::Row_t constraint_row() const;
    tabulate::Table::Row_t level_row() const;
    tabulate::Table::Row_t pebbled_row() const;
    tabulate::Table::Row_t length_row() const;
    // latex export
    tabulate::Table make_table() const override;
    tabulate::Table make_combined_table(const Run& control) const override;
  };

  class PebblingExperiment final : public expsuper::Experiment
  {
   public:
    PebblingExperiment(
        my::cli::ArgumentList const& a, PebblingModel& m, Logger& l);

   private:
    PebblingModel& ts;
    my::cli::model_t::Pebbling ts_descr;

    void reset_tables() override;
    std::shared_ptr<expsuper::Run> do_reps(const bool is_control) override;
  };
} // namespace pdr::pebbling::experiments

#endif // EXPERIMENTS_H
