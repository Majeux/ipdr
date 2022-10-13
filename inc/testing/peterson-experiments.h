#ifndef PETER_EXPERIMENTS_H
#define PETER_EXPERIMENTS_H

#include "cli-parse.h"
#include "logger.h"
#include "peterson-result.h"
#include "peterson.h"

#include <tabulate/table.hpp>

namespace pdr::peterson::experiments
{
  void peterson_run(PetersonModel& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);

  enum output_format
  {
    string,
    latex,
    markdown
  };
  struct Run
  {
    using Row_t   = tabulate::Table::Row_t;
    using Table_t = std::array<Row_t, 7>;
    std::string_view model;
    Tactic tactic;

    bool correct;
    double avg_time;
    double std_dev_time;

    Run(const my::cli::ArgumentList& args,
        const std::vector<PetersonResult>& r);
    std::string str(output_format fmt) const;
    std::string str_compared(const Run& other, output_format fmt) const;

   private:
    Table_t listing() const;
    Table_t combined_listing(const Run& other) const;
  };
} // namespace pdr::peterson::experiments

#endif
