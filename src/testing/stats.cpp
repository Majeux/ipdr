#include "stats.h"

namespace pdr
{
#warning TODO: add reduction average (%)
#warning TODO: add no. exceed MIC attempts (%)
#warning TODO: average no. MIC attempts
  Statistic::Statistic(bool t) : timed(t)
  {
    if (timed)
      total_time = 0.0;
    else
      total_time = {};
  }

  void Statistic::clear()
  {
    count.clear();
    time.clear();
    total_count = 0;
    if (timed)
      total_time = 0.0;
  }

  void Statistic::add(size_t i, size_t amount)
  {
    total_count += amount;

    while (count.size() <= i)
      count.push_back(0);

    count[i] += amount;
  }

  void Statistic::add_timed(size_t i, double dt)
  {
    assert(timed);
    assert(dt > 0.0);
    add(i);
    total_time.value() += dt;

    while (time.size() <= i)
      time.push_back(0.0);
    assert(count.size() == time.size());

    time[i] += dt;
  }

  std::optional<double> Statistic::avg_time(size_t i) const
  {
    if (time.size() <= i)
      return {};
    if (count[i] == 0)
      return {};
    return time[i] / count[i];
  }

  std::ostream& operator<<(std::ostream& out, const Statistic& stat)
  {
    using fmt::arg;
    using fmt::format;

    if (stat.timed)
      out << "# - total time:  " << stat.total_time.value() << std::endl;
    out << "# - total count: " << stat.total_count << std::endl;

    if (stat.timed)
    {
      for (size_t i = 0; i < stat.time.size(); i++)
      {
        auto avg_time = stat.avg_time(i);
        if (avg_time)
        {
          out << format("# - i={level:<3} {name:<10}: {state:<20} | "
                        "avg: {avg}",
                     arg("level", i), fmt::arg("name", "time"),
                     arg("state", stat.time[i]), arg("avg", avg_time.value()))
              << std::endl;
        }
        else
        {
          out << format("# - i={level:<3} {name:<10}: {state:<20} | "
                        "avg: {avg}",
                     arg("level", i), fmt::arg("name", "time"),
                     arg("state", stat.time[i]), arg("avg", "--"))
              << std::endl;
        }
      }
      out << "-" << std::endl;
    }
    for (size_t i = 0; i < stat.count.size(); i++)
      out << format("# - i={level:<3} {name:<10}: {state:<20}", arg("level", i),
                 arg("name", "calls"), arg("state", stat.count[i]))
          << std::endl;
    return out << "###";
  }

  // Statistics members
  //

  Statistics::Statistics(std::ofstream&& outfile)
        : file(std::move(outfile)), solver_calls(true), propagation_it(true),
          propagation_level(true), obligations_handled(true), ctis(true),
          subsumed_cubes(false)
    {
    }

    // set the statistics header to describe a DAG model for pebbling
     Statistics Statistics::PebblingStatistics(std::ofstream&& outfile, const dag::Graph& G)
    {
      Statistics s = Statistics(std::move(outfile));
      s.model_info.emplace("nodes", G.nodes.size());
      s.model_info.emplace("edges", G.edges.size());
      s.model_info.emplace("outputs", G.output.size());
      return s;
    }

    // set the statistics header to describe a DAG model for pebbling
     Statistics Statistics::PeterStatistics(std::ofstream&& outfile, unsigned p, unsigned N)
    {
      Statistics s = Statistics(std::move(outfile));
      s.model_info.emplace(PROC_STR, p);
      s.model_info.emplace(N_STR, N);
      return s;
    }
    
    void Statistics::update_peter(unsigned p, unsigned N)
    {
     model_info[PROC_STR] = p; 
     model_info[N_STR] = N; 
    }

    void Statistics::clear()
    {
      solver_calls.clear();
      propagation_it.clear();
      propagation_level.clear();
      obligations_handled.clear();
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

    std::ostream& operator<<(std::ostream& out, const Statistics& s)
    {
      assert(not s.model_info.empty());
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
          << "# Subsumed cubes" << std::endl
          << s.subsumed_cubes << std::endl
          << "#" << std::endl
          << "# Copied cubes" << std::endl
          << s.compute_copied() << " %" << std::endl
          << "#" << std::endl;

      return out << "######################" << std::endl;
    }
} // namespace pdr
