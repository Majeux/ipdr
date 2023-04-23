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
#include <string_view>
#include <vector>

namespace pdr
{
  struct Average
  {
    double total{ 0.0 };
    size_t count{ 0 };

    void add(double value)
    {
      total += value;
      count++;
    }

    operator double() const { return get(); }
    double get() const { return total / count; }

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

  struct TimedStatistic : public Statistic
  {
    double total_time = 0.0;
    std::vector<double> times;
    // unsigned total_count = 0; (from Statistic)
    // std::vector<unsigned> count; (from Statistic)

    void clear() override;
    void add(size_t i, double dt);

    std::optional<double> avg_time(size_t i) const;

    friend std::ostream& operator<<(
        std::ostream& out, TimedStatistic const& stat);
  };

  class Statistics
  {
   public:
    Statistic ctis;
    TimedStatistic solver_calls;
    TimedStatistic propagation_it;
    TimedStatistic propagation_level;
    TimedStatistic obligations_handled;
    TimedStatistic generalization;
    Average generalization_reduction;
    Average mic_attempts;
    unsigned mic_limit{ 0u };
    Statistic subsumed_cubes;

    double relax_copied_cubes_perc;
    std::vector<size_t> pre_relax_F;
    std::vector<size_t> post_relax_F;

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
    std::string graph_data() const;
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
  };

  // aggregated data over a certain number of experiments
  struct GraphData
  {
    std::vector<unsigned> counts;
    std::vector<double> times;
    std::vector<std::string> level_graphs;

    void append(Statistic const& s);
    void append(Statistic const& s, double time);
    void append(TimedStatistic const& s);
  };

  // snapshots of F pre- and post-relaxing for each experiment
  // and the copy rate between pre- and post-relaxing
  struct FrelaxData
  {
    std::vector<std::vector<size_t>> pre;  // repetition -> level -> n_cubes
    std::vector<std::vector<size_t>> post; // repetition -> level -> n_cubes
    std::vector<double> copyrate;          // percentage of copied levels

    // store relevant data from Satistics
    // return the number of frames in this data
    size_t append(Statistics const& stats);
  };

  class Graphs
  {
   public:
    void reset(std::string_view name);
    void add_datapoint(size_t label, Statistics const& stats);
    std::string get_cti() const;
    std::string get_obligation() const;
    std::string get_sat() const;
    std::string get_relax() const;
    std::string get_individual() const;
    // get relaxed percentage + frame breakdown

   private:
    std::string ts_name;
    std::map<unsigned, GraphData> cti_data; // parsed Statistics data
    std::map<unsigned, GraphData> obl_data;
    std::map<unsigned, GraphData> sat_data;
    std::map<unsigned, FrelaxData> relax_data;
    size_t no_frames{ 0 };

    std::string get(
        std::string_view name, std::map<unsigned, GraphData> const& data) const;
    std::string get(std::string_view name,
        std::map<unsigned, FrelaxData> const& data) const;

    static std::string barplot(std::string_view name);
    static std::string lineplot(std::string_view name);
    std::string relaxplot(std::string_view name) const;
    std::string relaxcontent(
        std::string_view filename, std::string_view data) const;
    std::string frames_data_line(
        size_t label, std::vector<std::vector<size_t>> const& data) const;

    std::vector<std::string> shared_options(
        std::optional<std::string_view> yname = " ") const;
    std::vector<std::string> bar_options(
        std::optional<std::string_view> yname = " ") const;
    std::vector<std::string> thinbar_options(
        std::optional<std::string_view> yname = " ") const;
    std::vector<std::string> line_options(
        std::optional<std::string_view> yname = " ") const;
  };
} // namespace pdr
#endif // STATS_H
