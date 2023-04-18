#include "stats.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <numeric>
#include <optional>
#include <sstream>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include "math.h"

namespace pdr
{
  using fmt::format;
  using std::endl;
  using std::string;
  using std::string_view;
  using std::vector;

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

    out << "# - total count: " << stat.total_count << endl;

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
    out << tabulate::MarkdownExporter().dump(t) << endl;

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

    out << "# - total time:  " << stat.total_time << endl;
    out << "# - total count: " << stat.total_count << endl;

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
    out << tabulate::MarkdownExporter().dump(t) << endl;

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
    ctis.clear();
    solver_calls.clear();
    propagation_it.clear();
    propagation_level.clear();
    obligations_handled.clear();
    generalization.clear();
    generalization_reduction.clear();
    subsumed_cubes.clear();
    relax_copied_cubes_perc = 0.0;
  }

  string Statistics::str() const
  {
    std::stringstream ss;
    ss << *this << endl;
    return ss.str();
  }

  void Statistics::write() { file << *this << endl; }

  std::ostream& operator<<(std::ostream& out, Statistics const& s)
  {
    out << "Model: " << endl << "--------" << endl;
    for (auto name_value : s.model_info)
      out << name_value.first << " = " << name_value.second << ", ";
    out << endl;
    out << "Total elapsed time: " << s.elapsed << endl << endl;

    out << endl;

    string frame_line("# - iter {level:<3} {name:<10}: {state:<20}");
    string frame_line_avg(
        "# - iter {level:<3} {name:<10}: {state:<20} | avg: {avg}");
    out << "######################" << endl
        << "# Statistics" << endl
        << "######################" << endl;

    out << "# Solver" << endl << s.solver_calls << endl;

    out << "# CTIs" << endl << s.ctis << endl;

    out << "# Obligations" << endl << s.obligations_handled << endl;

    out << "# Generalization" << endl
        << fmt::format(
               "## Mean reduction: {} %", s.generalization_reduction * 100.0)
        << endl
        << fmt::format("## Mean no. attempts in MIC: {}", s.mic_attempts.get())
        << endl
        << fmt::format("## No. limit-violations in MIC: {}", s.mic_limit)
        << endl
        << s.generalization << endl;

    out << "# Propagation per iteration" << endl << s.propagation_it << endl;

    out << "# Propagation per level" << endl << s.propagation_level << endl;

    out << "# Subsumed cubes" << endl << s.subsumed_cubes << endl;

    out << "#" << endl
        << "# Copied cubes during relax ipdr" << endl
        << s.relax_copied_cubes_perc << " %" << endl
        << "#" << endl;

    return out << "######################" << endl;
  }

  // GraphData MEMBERS
  //
  void GraphData::append(const Statistic& s)
  {
    counts.push_back(s.total_count);

    {
      string count_graph;
      for (size_t i{ 0 }; i < s.count.size(); i++)
        count_graph += format("({}, {})\n", i, s.count[i]);

      level_graphs.push_back(std::move(count_graph));
    }
  }

  void GraphData::append(const TimedStatistic& s)
  {
    counts.push_back(s.total_count);
    times.push_back(s.total_time);

    {
      assert(s.count.size() == s.times.size());
      string count_graph{ "### counts" };
      string time_graph{ "### times" };

      for (size_t i{ 0 }; i < s.count.size(); i++)
      {
        count_graph += format("({}, {})\n", i, s.count[i]);
        time_graph += format("({}, {})\n", i, s.times[i]);
      }

      level_graphs.push_back(count_graph + time_graph);
    }
  }

  template <typename NumericT>
  string pgf_line(size_t label, vector<NumericT> const& values)
  {
    static_assert(
        std::is_arithmetic<NumericT>::value, "NumericT is not a numeric type.");

    double avg = my::math::mean(values);
    double std_dev = my::math::std_dev(values, avg);

    return format("({},{}) +- (0, {})\n", label, avg, std_dev);
  }

  // Graphs MEMBERS
  //
  Graphs::Graphs() 
  {
    shared_options = {
      "ymode=log",
      "xtick=data",
      "xtick style={draw=none}",
      "xtick=data", 
      "xtick style={draw=none}",
      "minor tick num=1",
      "anchor=north,legend columns=-1}",
      "width=\textwidth",
      "enlarge x limits=0.1",
      "enlarge y limits={upper=0}",
    };
    bar_options = {
      "ybar",
      "ylabel={Count}",
      "bar width=7pt",
      "legend style={at={(0.1,0.98)}, anchor=north,legend columns=-1}",
    };
    line_options = {
      "axis y line*=right",
      "ylabel={Time (s)}",
      "legend style={at={(0.9,0.98)}, anchor=north,legend columns=-1}",
    };
  }
  void Graphs::add_datapoint(size_t label, Statistics const& stat)
  {
    {
      GraphData& cti_entry = cti_data.try_emplace(label).first->second;
      cti_entry.append(stat.ctis);
    }
    {
      GraphData& obl_entry = obl_data.try_emplace(label).first->second;
      obl_entry.append(stat.obligations_handled);
    }
    {
      GraphData& sat_entry = sat_data.try_emplace(label).first->second;
      sat_entry.append(stat.solver_calls);
    }
  }

  string Graphs::get() const
  {
    // add "symbolic x coords={0,1,2,...}" to options
    string count_bars = R"raw(\addplot+[
        bars/.cd,
        y dir=both,
        y explicit
    ] coordinates {
    )raw";
    string time_lines = R"raw(\addplot+[
        bars/.cd,
        axis lines*=right,
        y explicit
    ] coordinates {
    )raw";


    for (auto& [i, data] : cti_data)
    {
      count_bars += pgf_line(i, data.counts);
      count_bars += pgf_line(i, data.times);
    }
  }
} // namespace pdr
