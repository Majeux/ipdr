#ifndef STATS_H
#define STATS_H

#include "dag.h"
#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace pdr
{
  struct Statistic
  {
    const bool timed;

    unsigned total_count = 0;
    std::vector<unsigned> count;

    std::optional<double> total_time = 0.0;
    std::vector<double> time;

    Statistic(bool t = false);

    void clear();
    void add(size_t i, size_t amount = 1);
    void add_timed(size_t i, double dt);
    std::optional<double> avg_time(size_t i) const;

    friend std::ostream& operator<<(std::ostream& out, const Statistic& stat);
  };

  class Statistics
  {
   private:
    std::ofstream file;
    std::map<std::string, unsigned> model_info;

    static inline const std::string PROC_STR = "processes";
    static inline const std::string N_STR = "max_processes";

    double compute_copied() const
    {
      if (copied_cubes.total != 0)
        return 0;
      return ((double)copied_cubes.count / copied_cubes.total) * 100.0;
    }

   public:
    Statistic solver_calls;
    Statistic propagation_it;
    Statistic propagation_level;
    Statistic obligations_handled;

    Statistic ctis;
    Statistic subsumed_cubes;
    struct
    {
      unsigned count{ 0 };
      unsigned total{ 0 };
    } copied_cubes;

    double elapsed = -1.0;
    std::vector<std::string> solver_dumps;

    // Statistics takes ownership of its file stream
    Statistics(std::ofstream&& f);

    // set the statistics header to describe a DAG model for pebbling
    static Statistics PebblingStatistics(std::ofstream&& f, const dag::Graph& G);
    // set the statistics header to describe a DAG model for pebbling
    static Statistics PeterStatistics(std::ofstream&& f, unsigned p, unsigned N);
    // update the current and maximum processes in the peterson header
    void update_peter(unsigned p, unsigned N);
    void clear();
    std::string str() const;
    void write();
    friend std::ostream& operator<<(std::ostream& out, const Statistics& s);

    template <typename... Args> void write(std::string_view s, Args&&... a)
    {
      file << fmt::format(s, std::forward<Args>(a)...) << std::endl;
    }

  };
} // namespace pdr
#endif // STATS_H
