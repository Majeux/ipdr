#ifndef PETER_EXPERIMENTS_H
#define PETER_EXPERIMENTS_H

#include "cli-parse.h"
#include "experiments.h"
#include "logger.h"
#include "pdr-context.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"

#include <memory>
#include <tabulate/table.hpp>

namespace pdr::peterson::experiments
{
  namespace expsuper = ::pdr::experiments;

  void peterson_run(PetersonModel& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);

  enum output_format
  {
    string,
    latex,
    markdown
  };

  class PeterRun final : public pdr::experiments::Run
  {
   public:
    using Row_t = tabulate::Table::Row_t;

    bool correct;

    PeterRun(std::string const& t, std::string const& m,
        std::vector<PetersonResult> const& r);

   private:
    tabulate::Table::Row_t correct_row() const;
    tabulate::Table make_table() const override;
    tabulate::Table make_combined_table(const Run& control) const override;
  };

  class PeterExperiment : pdr::experiments::Experiment
  {
   public:
    PeterExperiment(
        my::cli::ArgumentList const& a, PetersonModel& m, Logger& l);

   private:
    PetersonModel& ts;
    my::cli::model_t::Peterson ts_descr;

    std::unique_ptr<expsuper::Run> single_run(bool is_control) override;
  };
} // namespace pdr::peterson::experiments

#endif
