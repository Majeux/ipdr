#include "stats.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <optional>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>

namespace pdr
{
#warning TODO: add no. exceed MIC attempts (%)
#warning TODO: average no. MIC attempts
  void Statistic::clear()
  {
    count.clear();
    total_count = 0;
  }

  void Statistic::add(size_t i, size_t amount)
  {
    total_count += amount;

    while (count.size() <= i)
      count.push_back(0);

    count[i] += amount;
  }

  std::ostream& operator<<(std::ostream& out, Statistic const& stat)
  {
    using fmt::arg;
    using fmt::format;
    using Row_t = tabulate::Table::Row_t;

    out << "# - total count: " << stat.total_count << std::endl;

    tabulate::Table t;
    {
      Row_t header = { "level", "count" };
      t.add_row(header);

      for (size_t i = 0; i < stat.count.size(); i++)
      {
        Row_t row;
        row.push_back(fmt::to_string(i));
        row.push_back(fmt::to_string(stat.count[i]));

        t.add_row(row);
      }
    }
    out << tabulate::MarkdownExporter().dump(t) << std::endl;

    return out << "###";
  }

  void TimedStatistic::clear()
  {
    Statistic::clear();
    total_time = 0.0;
    times.clear();
  }

  void TimedStatistic::add(size_t i, double dt)
  {
    assert(dt > 0.0);
    assert(times.size() == count.size());

    Statistic::add(i);
    total_time += dt;

    while (times.size() <= i)
      times.push_back(0.0);
    assert(count.size() == times.size());

    times[i] += dt;
  }

  std::optional<double> TimedStatistic::avg_time(size_t i) const
  {
    if (times.size() <= i)
      return {};
    if (count[i] == 0)
      return {};
    return times[i] / count[i];
  }

  std::ostream& operator<<(std::ostream& out, TimedStatistic const& stat)
  {
    using fmt::arg;
    using fmt::format;
    using Row_t = tabulate::Table::Row_t;

    out << "# - total time:  " << stat.total_time << std::endl;
    out << "# - total count: " << stat.total_count << std::endl;

    tabulate::Table t;
    {
      Row_t header = { "level", "count", "time", "avg. time" };
      t.add_row(header);

      assert(stat.count.size() == stat.times.size());

      for (size_t i = 0; i < stat.times.size(); i++)
      {
        Row_t row;
        row.push_back(fmt::to_string(i));
        row.push_back(fmt::to_string(stat.count[i]));
        row.push_back(fmt::to_string(stat.times[i]));
        if (auto avg_time = stat.avg_time(i))
          row.push_back(fmt::to_string(avg_time.value()));
        else
          row.push_back("-");

        t.add_row(row);
      }
    }
    out << tabulate::MarkdownExporter().dump(t) << std::endl;

    return out << "###";
  }

  // Statistics members
  //

  Statistics::Statistics(std::ofstream&& outfile) : file(std::move(outfile)) {}

  void Statistics::is_pebbling(dag::Graph const& G)
  {
    assert(!finished);
    model_info.emplace("nodes", G.nodes.size());
    model_info.emplace("edges", G.edges.size());
    model_info.emplace("outputs", G.output.size());
    finished = true;
  }

  // set the statistics header to describe a DAG model for pebbling
  void Statistics::is_peter(unsigned p, unsigned N)
  {
    assert(!finished);
    model_info.emplace(PROC_STR, p);
    model_info.emplace(N_STR, N);
    finished = true;
  }

  void Statistics::update_peter(unsigned p, unsigned N)
  {
    model_info[PROC_STR] = p;
    model_info[N_STR]    = N;
  }

  void Statistics::clear()
  {
    solver_calls.clear();
    propagation_it.clear();
    propagation_level.clear();
    obligations_handled.clear();
    generalization.clear();
    generalization_reduction.clear();
    ctis.clear();
    subsumed_cubes.clear();
    copied_cubes = { 0, 0 };
  }

  std::string Statistics::str() const
  {
    std::stringstream ss;
    ss << *this << std::endl;
    return ss.str();
  }

  void Statistics::write() { file << *this << std::endl; }

  std::ostream& operator<<(std::ostream& out, Statistics const& s)
  {
    out << "Model: " << std::endl << "--------" << std::endl;
    for (auto name_value : s.model_info)
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

    out << "# Solver" << std::endl << s.solver_calls << std::endl;

    out << "# CTIs" << std::endl << s.ctis << std::endl;

    out << "# Obligations" << std::endl << s.obligations_handled << std::endl;

    out << "# Generalization" << std::endl
        << fmt::format("## Mean reduction: {} %",
               s.generalization_reduction.get() * 100.0)
        << std::endl
        << fmt::format("## Mean no. attempts in MIC: {}", s.mic_attempts.get())
        << std::endl
        << fmt::format("## No. limit-violations in MIC: {}", s.mic_limit)
        << std::endl
        << s.generalization << std::endl;

    out << "# Propagation per iteration" << std::endl
        << s.propagation_it << std::endl;

    out << "# Propagation per level" << std::endl
        << s.propagation_level << std::endl;

    out << "# Subsumed cubes" << std::endl << s.subsumed_cubes << std::endl;

    out << "#" << std::endl
        << "# Copied cubes" << std::endl
        << s.compute_copied() << " %" << std::endl
        << "#" << std::endl;

    return out << "######################" << std::endl;
  }
} // namespace pdr
