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
  template <typename T> struct Average
  {
    T total      = 0;
    size_t count = 0;

    void add(T value)
    {
      total += value;
      count++;
    }

    T get() const { return total / count; }

    void clear()
    {
      total = 0;
      count = 0;
    }
  };

  struct Statistic
  {
    unsigned total_count = 0;
    std::vector<unsigned> count;

    virtual void clear();
    void add(size_t i, size_t amount = 1);

    friend std::ostream& operator<<(std::ostream& out, Statistic const& stat);
  };

  struct TimedStatistic : private Statistic
  {
    double total_time = 0.0;
    std::vector<double> times;

    void clear() override;
    void add(size_t i, double dt);

    std::optional<double> avg_time(size_t i) const;

    friend std::ostream& operator<<(
        std::ostream& out, TimedStatistic const& stat);
  };

  class Statistics
  {
   public:
    TimedStatistic solver_calls;
    TimedStatistic propagation_it;
    TimedStatistic propagation_level;
    TimedStatistic obligations_handled;
    TimedStatistic generalization;
    Average<double> generalization_reduction;

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
    void is_pebbling(dag::Graph const& G);
    // set the statistics header to describe a DAG model for pebbling
    void is_peter(unsigned p, unsigned N);

    // update the current and maximum processes in the peterson header
    void update_peter(unsigned p, unsigned N);
    void clear();
    std::string str() const;
    void write();
    friend std::ostream& operator<<(std::ostream& out, Statistics const& s);

    template <typename... Args> void write(std::string_view s, Args&&... a)
    {
      file << fmt::format(s, std::forward<Args>(a)...) << std::endl;
    }

   private:
    bool finished{ false };
    std::ofstream file;
    std::map<std::string, unsigned> model_info;

    static inline const std::string PROC_STR = "processes";
    static inline const std::string N_STR    = "max_processes";

    double compute_copied() const
    {
      if (copied_cubes.total != 0)
        return 0;
      return ((double)copied_cubes.count / copied_cubes.total) * 100.0;
    }
  };
} // namespace pdr
#endif // STATS_H
