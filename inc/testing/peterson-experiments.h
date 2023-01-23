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

    PeterRun(std::string const& m, std::string const& t,
        std::vector<std::unique_ptr<IpdrResult>>&& results);

   private:
    tabulate::Table::Row_t correct_row() const;
    tabulate::Table make_table() const override;
    tabulate::Table make_combined_table(const Run& control) const override;
  };

  class PetersonExperiment : public expsuper::Experiment
  {
   public:
    PetersonExperiment(
        my::cli::ArgumentList const& a, PetersonModel& m, Logger& l);

   private:
    PetersonModel& ts;
    my::cli::model_t::Peterson ts_descr;

    void reset_tables() override;
    std::shared_ptr<expsuper::Run> do_reps(const bool is_control) override;
  };
} // namespace pdr::peterson::experiments

#endif
