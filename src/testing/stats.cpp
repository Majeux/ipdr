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

} // namespace pdr
