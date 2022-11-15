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

  void pebbling_run(pebbling::PebblingModel& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);

  struct PebblingRun final : public ::pdr::experiments::Run
  {
    using Row_t   = tabulate::Table::Row_t;
    // 7 rows

    std::optional<PebblingResult::PebblingInvariant> min_inv;
    std::optional<PebblingResult::PebblingTrace> min_strat;

    PebblingRun(std::string const& t, std::string const& m,
        const std::vector<pebbling::PebblingResult>& results);

   private:
    tabulate::Table::Row_t constraint_row() const;
    tabulate::Table::Row_t level_row() const;
    tabulate::Table::Row_t pebbled_row() const;
    tabulate::Table::Row_t length_row() const;
    // latex export
    tabulate::Table make_table() const override;
    tabulate::Table make_combined_table(const Run& control) const override;
  };

  class PebblingExperiment final : pdr::experiments::Experiment
  {
   public:
    PebblingExperiment(
        my::cli::ArgumentList const& a, PebblingModel& m, Logger& l);

   private:
    PebblingModel& ts;
    my::cli::model_t::Peterson ts_descr;

    std::unique_ptr<expsuper::Run> single_run(bool is_control) override;
  };
} // namespace pdr::pebbling::experiments

#endif // EXPERIMENTS_H
