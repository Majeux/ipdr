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
    void append(TimedStatistic const& s);
  };

  class Graphs
  {
   public:
     void reset();
    void add_datapoint(size_t label, Statistics const& stat);
    std::string get_cti() const;
    std::string get_obligation() const;
    std::string get_sat() const;
    std::string get_individual() const;
    // get relaxed percentage + frame breakdown

   private:
    std::map<unsigned, GraphData> cti_data; // parsed Statistics data
    std::map<unsigned, GraphData> obl_data;
    std::map<unsigned, GraphData> sat_data;

    std::string get(std::map<unsigned, GraphData> const& data) const;

    static std::string barplot(std::string_view name);
    static std::string lineplot(std::string_view name);

    // barplot with error bars
    // static constexpr const char* barplot =
    //     "\\addplot+ [error bars/.cd, y dir=both, y explicit] "
    //     "table [x=x, y=y, y error=err] {bar.dat};";

    // line plot (-x-) with filled in error range
    // static constexpr const char* lineplot =
    //     "\\addplot+[mark=x, color=red, mark size=4pt, x=x, y=y] "
    //     "table {line.dat};\n"
    //     "\\addplot [name path=upper,draw=none]\n"
    //     "    table[x=x,y expr=\\thisrow{y}+\\thisrow{err}] {line.dat};\n"
    //     "\\addplot [name path=lower,draw=none] \n"
    //     "    table[x=x,y expr=\\thisrow{y}-\\thisrow{err}] {line.dat};\n"
    //     "\\addplot [fill=gray!50] fill between[of=upper and lower];";

    static constexpr std::initializer_list<const char*> shared_options{
      // "ymode=log",
      // "ymin=0.01",
      "xtick=data",
      "xtick style={draw=none}",
      "minor tick num=1",
      "width=\\textwidth",
      "enlarge x limits=0.1",
      "enlarge y limits={upper=0}",
    };
    static constexpr std::initializer_list<const char*> bar_options{
      "ybar",
      "ylabel={Count}",
      "bar width=7pt",
      "legend style={at={(0.1,0.98)}, anchor=north,legend columns=-1}",
    };
    static constexpr std::initializer_list<const char*> line_options{
      "axis y line*=right",
      "ylabel={Time (s)}",
      "legend style={at={(0.9,0.98)}, anchor=north,legend columns=-1}",
    };

  };
} // namespace pdr
#endif // STATS_H
