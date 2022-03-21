#ifndef STATS_H
#define STATS_H

#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace pdr
{
  struct Statistic
  {
    bool timed;

    Statistic(bool t = false) : timed(t) {}

    unsigned total_count = 0;
    std::vector<unsigned> count;
    // optional
    double total_time = 0;
    std::vector<double> time;

    void add(size_t i, size_t amount = 1)
    {
      total_count += amount;

      while (count.size() <= i)
        count.push_back(0);

      count[i] += amount;
    }

    void add_timed(size_t i, double dt)
    {
      assert(timed);
      assert(dt > 0.0);
      add(i);
      total_time += dt;

      while (time.size() <= i)
        time.push_back(0.0);
      assert(count.size() == time.size());

      time[i] += dt;
    }

    double avg_time(size_t i) const
    {
      if (time.size() <= i)
        return -1.0;
      return time[i] / count[i];
    }

    friend std::ostream& operator<<(std::ostream& out, const Statistic& stat)
    {
      if (stat.timed)
        out << "# - total time:  " << stat.total_time << std::endl;
      out << "# - total count: " << stat.total_count << std::endl;

      if (stat.timed)
      {
        for (size_t i = 0; i < stat.time.size(); i++)
        {
          out << fmt::format("# - i={level:<3} {name:<10}: {state:<20} | "
                             "avg: {avg}",
                             fmt::arg("level", i), fmt::arg("name", "time"),
                             fmt::arg("state", stat.time[i]),
                             fmt::arg("avg", stat.avg_time(i)))
              << std::endl;
        }
        out << "-" << std::endl;
      }
      for (size_t i = 0; i < stat.count.size(); i++)
        out << fmt::format("# - i={level:<3} {name:<10}: {state:<20}",
                           fmt::arg("level", i), fmt::arg("name", "calls"),
                           fmt::arg("state", stat.count[i]))
            << std::endl;
      return out << "###";
    }
  };

  class Statistics
  {

   public:
    Statistic solver_calls;
    Statistic propagation_it;
    Statistic propagation_level;
    Statistic obligations_handled;

    Statistic subsumed_cubes;

    double elapsed = -1.0;
    std::map<std::string, unsigned> model;

    Statistics()
        : solver_calls(true), propagation_it(true), propagation_level(true),
          obligations_handled(true), subsumed_cubes(false)
    {
    }

    std::string str() const
    {
      std::stringstream ss;
      ss << *this << std::endl;
      return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const Statistics& s)
    {
      assert(!s.model.empty());
      if (!s.model.empty())
      {
        out << "Model: " << std::endl << "--------" << std::endl;
        for (auto name_value : s.model)
          out << name_value.first << " = " << name_value.second << ", ";
        out << std::endl;
      }
      out << "Total elapsed time: " << s.elapsed << std::endl << std::endl;

      out << std::endl;

      std::string frame_line("# - iter {level:<3} {name:<10}: {state:<20}");
      std::string frame_line_avg(
          "# - iter {level:<3} {name:<10}: {state:<20} | avg: {avg}");
      out << "######################" << std::endl
          << "# Statistics" << std::endl
          << "######################" << std::endl;

      out << "# Solver" << std::endl << s.solver_calls << std::endl;
      out << "# Obligations" << std::endl << s.obligations_handled << std::endl;
      out << "# Propagation per iteration" << std::endl
          << s.propagation_it << std::endl;
      out << "# Propagation per level" << std::endl
          << s.propagation_level << std::endl;
      out << "# Subsumed clauses" << std::endl << s.subsumed_cubes << std::endl;
      out << "#" << std::endl;

      return out << "######################" << std::endl;
    }
  };
} // namespace pdr
#endif // STATS_H
