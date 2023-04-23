#include "stats.h"
#include "math.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
#include <initializer_list>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>

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
    pre_relax_F.clear();
    post_relax_F.clear();
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

    string count_graph;
    for (size_t i{ 0 }; i < s.count.size(); i++)
      count_graph += format("({}, {})\n", i, s.count[i]);

    level_graphs.push_back(std::move(count_graph));
  }

  void GraphData::append(const Statistic& s, double total_time)
  {
    append(s);
    times.push_back(total_time);
  }

  void GraphData::append(const TimedStatistic& s)
  {
    counts.push_back(s.total_count);
    times.push_back(s.total_time);

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

  // FrelaxData MEMBERS
  //
  size_t FrelaxData::append(const Statistics& stats)
  {
    assert(pre.size() == post.size() && pre.size() == copyrate.size());

    pre.push_back(stats.pre_relax_F);
    post.push_back(stats.post_relax_F);
    copyrate.push_back(stats.relax_copied_cubes_perc);

    assert(stats.pre_relax_F.size() == stats.post_relax_F.size());

    return stats.pre_relax_F.size();
  }

  // Graphs MEMBERS
  //
  void Graphs::reset(string_view name)
  {
    ts_name = name;
    cti_data.clear();
    obl_data.clear();
    cti_data.clear();
    relax_data.clear();
    no_frames = 0;
  }

  void Graphs::add_datapoint(size_t label, Statistics const& stats)
  {
    {
      GraphData& cti_entry = cti_data.try_emplace(label).first->second;
      cti_entry.append(stats.ctis, stats.elapsed);
    }
    {
      GraphData& obl_entry = obl_data.try_emplace(label).first->second;
      obl_entry.append(stats.obligations_handled);
    }
    {
      GraphData& sat_entry = sat_data.try_emplace(label).first->second;
      sat_entry.append(stats.solver_calls);
    }
    {
      FrelaxData& relax_entry = relax_data.try_emplace(label).first->second;
      no_frames               = std::max(no_frames, relax_entry.append(stats));
    }
  }

  string Graphs::get_cti() const
  {
    return "\% CTI graph\n" + get("cti", cti_data);
  }

  string Graphs::get_obligation() const
  {
    return "\% Obligation graph\n" + get("obligation", obl_data);
  }

  string Graphs::get_sat() const
  {
    return "\% Sat-call graph\n" + get("SAT", sat_data);
  }

  string Graphs::get_relax() const
  {
    return "\% Frames after relaxing\n" + get("relax-frames", relax_data);
  }

  namespace // pgfplot build functions
  {
    std::map<char, string> escape_char{
      { '_', "\\_" },
      { '^', "\\^{}" },
    };

    string escape(string_view s)
    {
      string rv;
      for (char c : s)
      {
        auto replace = escape_char.find(c);
        if (replace != escape_char.end())
          rv.append(replace->second);
        else
          rv.push_back(c);
      }
      return rv;
    }

    string tikzpicture(std::string_view content)
    {
      std::stringstream ss;
      ss << "\\begin{tikzpicture}" << endl
         << content << endl
         << "\\end{tikzpicture}" << endl;
      return ss.str();
    }

    string axis(vector<std::vector<std::string>> const& opt_groups,
        string_view name, string_view content)
    {
      std::stringstream ss;

      ss << "\\begin{axis}" << endl << "[" << endl;
      for (vector<string> const& opts : opt_groups)
        for (string_view option : opts)
          ss << "    " << option << "," << endl;
      ss << "]" << endl;

      ss << format("\\legend{{{}}}", escape(name)) << endl
         << content << endl
         << "\\end{axis}" << endl;

      return ss.str();
    }

    string filecontents(string_view filename, string_view data)
    {
      std::stringstream ss;
      ss << format("\\begin{{filecontents}}{{{}.dat}}", filename) << endl
         << "    x y err" << endl
         << data << "\\end{filecontents}" << endl;

      return ss.str();
    }

    template <typename NumericT>
    string pgf_line(size_t label, vector<NumericT> const& values)
    {
      static_assert(std::is_arithmetic<NumericT>::value,
          "NumericT is not a numeric type.");

      double avg     = my::math::mean(values);
      double std_dev = my::math::std_dev(values, avg);

      return format("    {} {} {}\n", label, avg, std_dev);
    }

  } // namespace

  string Graphs::get(
      string_view name, std::map<unsigned, FrelaxData> const& data) const
  {
    string predata, postdata, ratedata;
    string prename  = format("{}-{}-pre", ts_name, name);
    string postname = format("{}-{}-post", ts_name, name);
    string ratename = format("{}-{}-perc", ts_name, name);

    for (auto& [i, d] : data)
    {
      predata += frames_data_line(i, d.pre);
      postdata += frames_data_line(i, d.post);
      ratedata += pgf_line(i, d.copyrate);
    }

    return tikzpicture(
               axis({ shared_options("Frames per Constraint"),
                        thinbar_options("No. cubes") },
                   prename,
                   relaxcontent(prename, predata) + relaxplot(prename)) +
               axis({ shared_options(), line_options("Copyrate (\\%)") },
                   ratename,
                   filecontents(ratename, ratedata) + lineplot(ratename))) +
           tikzpicture(
               axis({ shared_options("Frames per Constraint"),
                        thinbar_options("No. cubes") },
                   postname,
                   relaxcontent(postname, postdata) + relaxplot(postname)) +
               axis({ shared_options(), line_options("Copyrate (\\%)") },
                   ratename,
                   filecontents(ratename, ratedata) + lineplot(ratename)));
  }

  string Graphs::get(
      string_view name, std::map<unsigned, GraphData> const& data) const
  {
    string count_data, line_data;
    string barname  = format("{}-{}-count", ts_name, name);
    string linename = format("{}-{}-time", ts_name, name);

    for (auto& [i, d] : data)
    {
      count_data += pgf_line(i, d.counts);
      line_data += pgf_line(i, d.times);
    }

    return tikzpicture(
        axis({ shared_options("Constraints"), bar_options("Count") }, barname,
            filecontents(barname, count_data) + barplot(barname)) +
        axis({ shared_options(), line_options("Time (s)") }, linename,
            filecontents(linename, line_data) + lineplot(linename)));
  }

  vector<string> Graphs::shared_options(optional<string_view> xname) const
  {
    vector<string> rv = {
      // "ymode=log",
      // "ymin=0.01",
      "xtick=data",
      "xtick style={draw=none}",
      "minor tick num=1",
      "width=\\textwidth",
      "enlarge x limits=0.1",
      "enlarge y limits={upper=0}",
    };

    if (xname)
      rv.push_back(format("xlabel={{{}}}", *xname));

    return rv;
  }

  std::vector<string> Graphs::bar_options(optional<string_view> yname) const
  {
    vector<string> rv = {
      "ybar",
      "axis y line*=left",
      "bar width=7pt",
      "legend style={at={(0.35,0.95)}, anchor=east,legend columns=-1}",
    };

    if (yname)
      rv.push_back(format("ylabel={{{}}}", *yname));

    return rv;
  }

  std::vector<string> Graphs::thinbar_options(optional<string_view> yname) const
  {
    vector<string> rv = {
      "ybar=1pt",
      "axis y line*=left",
      "bar width=0pt",
      "legend style={at={(0.35,0.95)}, anchor=east,legend columns=-1}",
    };

    if (yname)
      rv.push_back(format("ylabel={{{}}}", *yname));

    return rv;
  }

  std::vector<string> Graphs::line_options(optional<string_view> yname) const
  {
    vector<string> rv = {
      "axis y line*=right",
      "legend style={at={(0.35,0.88)}, anchor=east,legend columns=-1}",
      "xticklabels={,,}", // labels already displayed by bargraph
    };

    if (yname)
      rv.push_back(format("ylabel={{{}}}", *yname));

    return rv;
  }

  string Graphs::barplot(string_view name)
  {
    return format("\\addplot+ [error bars/.cd, y dir=both, y explicit]\n"
                  "    table [x=x, y=y, y error=err] {{{}.dat}};",
        name);
  }

  string Graphs::lineplot(string_view name)
  {
    return format(
        "\\addplot+[mark=x, color=red, mark size=4pt, x=x, y=y] "
        "table {{{}.dat}};\n"
        "\\addplot [name path=upper,draw=none]\n"
        "    table[x=x,y expr=\\thisrow{{y}}+\\thisrow{{err}}] {{{}.dat}};\n"
        "\\addplot [name path=lower,draw=none] \n"
        "    table[x=x,y expr=\\thisrow{{y}}-\\thisrow{{err}}] {{{}.dat}};\n"
        "\\addplot [fill=gray!50] fill between[of=upper and lower];",
        name, name, name);
  }

  string Graphs::relaxplot(string_view name) const
  {
    std::stringstream ss;

    for (size_t i{ 1 }; i < no_frames; i++)
    {
      // ss << format("\\addplot [error bars/.cd, y dir=both, y explicit]\n"
      //              "    table [x=x, y=f{}, y error=f{}err] {{{}.dat}};",
      //           i, i, name)
      ss << format("\\addplot [draw=black, fill=black]\n"
                   "    table [x=x, y=f{}, y error=f{}err] {{{}.dat}};",
                i, i, name)
         << endl;
    }

    return ss.str();
  }

  string Graphs::relaxcontent(string_view filename, string_view data) const
  {
    std::stringstream ss;
    ss << format("\\begin{{filecontents}}{{{}.dat}}", filename) << endl
       << "    x";

    for (size_t i{ 1 }; i < no_frames; i++)
      ss << format(" f{} f{}err", i, i);

    ss << endl << data << "\\end{filecontents}" << endl;

    return ss.str();
  }

  // data:repetition -> level -> n_cubes
  // format = x f1 f1err f2 f2err ...
  string Graphs::frames_data_line(
      size_t label, vector<vector<size_t>> const& data) const
  {
    string rv = format("    {}", label);

    size_t max_size = std::max_element(data.cbegin(), data.cend(),
        [](vector<size_t> const& a, vector<size_t> const& b) {
          return a.size() < b.size();
        })->size();

    for (size_t i{ 1 }; i < no_frames; i++) // iterate levels
    {
      vector<size_t> level_values; // aggregate repetitions per level
      for (size_t j{ 0 }; j < data.size(); j++)
        level_values.push_back(i < max_size ? data[j].at(i) : 0);

      double avg     = my::math::mean(level_values);
      double std_dev = my::math::std_dev(level_values, avg);
      rv += format(" {} {}", avg, std_dev);
    }
    return rv + "\n";
  }

} // namespace pdr
