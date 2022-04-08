#ifndef STATS_H
#define STATS_H

#include "dag.h"
#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
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

   public:
    Statistic solver_calls;
    Statistic propagation_it;
    Statistic propagation_level;
    Statistic obligations_handled;

    Statistic ctis;
    Statistic subsumed_cubes;
#warning TODO: add reduction average (%)

    double elapsed = -1.0;
    std::map<std::string, unsigned> model;
    std::vector<std::string> solver_dumps;

    Statistics(std::ofstream&& f, const dag::Graph& G)
        : file(std::move(f)), solver_calls(true), propagation_it(true),
          propagation_level(true), obligations_handled(true), ctis(true),
          subsumed_cubes(false)
    {
      model.emplace("nodes", G.nodes.size());
      model.emplace("edges", G.edges.size());
      model.emplace("outputs", G.output.size());
    }

    void clear()
    {
      solver_calls.clear();
      propagation_it.clear();
      propagation_level.clear();
      obligations_handled.clear();
      ctis.clear();
    }

    std::string str() const
    {
      std::stringstream ss;
      ss << *this << std::endl;
      return ss.str();
    }

    template <typename... Args> void write(std::string_view s, Args&&... a)
    {
      file << fmt::format(s, std::forward<Args>(a)...) << std::endl;
    }

    void write() { file << *this << std::endl; }

    friend std::ostream& operator<<(std::ostream& out, const Statistics& s)
    {
      assert(not s.model.empty());
      out << "Model: " << std::endl << "--------" << std::endl;
      for (auto name_value : s.model)
        out << name_value.first << " = " << name_value.second << ", ";
      out << std::endl;
      out << "Total elapsed time: " << s.elapsed << std::endl << std::endl;

      out << std::endl;

      std::string frame_line("# - iter {level:<3} {name:<10}: {state:<20}");
      std::string frame_line_avg(
          "# - iter {level:<3} {name:<10}: {state:<20} | avg: {avg}");
      out << "######################" << std::endl
          << "# Statistics" << std::endl
          << "######################" << std::endl;

      out << "# Solver" << std::endl
          << s.solver_calls << std::endl
          << "# CTIs" << std::endl
          << s.ctis << std::endl
          << "# Obligations" << std::endl
          << s.obligations_handled << std::endl
          << "# Propagation per iteration" << std::endl
          << s.propagation_it << std::endl
          << "# Propagation per level" << std::endl
          << s.propagation_level << std::endl
          << "# Subsumed clauses" << std::endl
          << s.subsumed_cubes << std::endl
          << "#" << std::endl;

      return out << "######################" << std::endl;
    }
  };
} // namespace pdr
#endif // STATS_H
