#ifndef STAT
#define STAT

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
    using fmt::arg;
    using fmt::format;
    using std::endl;
    using std::string;
    using std::vector;

    struct Statistic
    {
        bool timed;

        Statistic(bool t = false) : timed(t) {}

        unsigned total_count = 0;
        vector<unsigned> count;
        // optional
        double total_time = 0;
        vector<double> time;

        void add(size_t i, size_t amount = 1)
        {
            total_count += amount;

            while (count.size() <= i)
                count.push_back(0);

            count[i] += amount;
        }

        void add_timed(size_t i, double dt)
        {
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

        friend std::ostream& operator<<(std::ostream& out,
                                        const Statistic& stat)
        {
            if (stat.timed)
                out << "# - total time:  " << stat.total_time << endl;
            out << "# - total calls: " << stat.total_count << endl;

            if (stat.timed)
            {
                for (size_t i = 0; i < stat.time.size(); i++)
                    out << format(
                               "# - iter {level:<3} {name:<10}: {state:<20} | "
                               "avg: {avg}",
                               arg("level", i), arg("name", "time"),
                               arg("state", stat.time[i]),
                               arg("avg", stat.avg_time(i)))
                        << endl;
            }
            for (size_t i = 0; i < stat.count.size(); i++)
                out << format("# - iter {level:<3} {name:<10}: {state:<20}",
                              arg("level", i), arg("name", "calls"),
                              arg("state", stat.count[i]))
                    << endl;
            return out << "#";
        }
    };

    class Statistics
    {

      public:
        Statistic solver_calls;
        Statistic propagation;
        Statistic obligations_handled;

        Statistic subsumed_cubes;

        double elapsed = -1.0;
        std::map<string, unsigned> model;

        Statistics()
            : solver_calls(true), propagation(true), obligations_handled(true)
        {
        }

        std::string to_string() const
        {
            std::stringstream ss;
            ss << *this << endl;
            return ss.str();
        }

        friend std::ostream& operator<<(std::ostream& out, const Statistics& s)
        {
            out << "Total elapsed time: " << s.elapsed << endl << endl;

            if (!s.model.empty())
            {
                out << "Model: " << endl << "--------" << endl;
                for (auto name_value : s.model)
                    out << name_value.first << " = " << name_value.second
                        << endl;
            }
            out << endl;

            string frame_line("# - iter {level:<3} {name:<10}: {state:<20}");
            string frame_line_avg(
                "# - iter {level:<3} {name:<10}: {state:<20} | avg: {avg}");
            out << "######################" << endl
                << "# Statistics" << endl
                << "######################" << endl;

            out << "# Solver" << endl << s.solver_calls << endl;
            out << "# Obligations" << endl << s.obligations_handled << endl;
            out << "# Propagation" << endl << s.propagation << endl;
            out << "# Subsumed clauses" << endl << s.subsumed_cubes << endl;
            out << "#" << endl;

            return out << "######################" << endl;
        }
    };
} // namespace pdr
#endif // STAT
