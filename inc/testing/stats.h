#ifndef STAT
#define STAT

#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <ostream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <fmt/format.h>

namespace pdr {
	using std::vector;
	using std::string;
	using std::endl;
	using fmt::format;
	using fmt::arg;

	class Statistics
	{

		public:
			unsigned total_solver_calls = 0;
			double total_solver_time = 0;
			vector<unsigned> solver_calls;
			vector<double> solver_time;

			unsigned total_obligations_handled = 0;
			double total_obligations_time = 0;
			vector<unsigned> obligations_handled;
			vector<double> obligations_time;
			
			unsigned total_subsumed_cubes = 0;
			vector<unsigned> subsumed_cubes;

			unsigned blocked_ignored = 0;

			void solver_call(size_t frame)
			{
				total_solver_calls++;

				while (solver_calls.size() <= frame)
					solver_calls.push_back(0);

				solver_calls[frame]++;
			}

			void solver_call(size_t frame, double time)
			{
				assert(time > 0.0);
				solver_call(frame);
				total_solver_time += time;

				while (solver_time.size() <= frame)
					solver_time.push_back(0.0);
				assert(solver_calls.size() == solver_time.size());

				solver_time[frame] += time;
			}

			double avg_solver_time(size_t frame) const
			{
				if (solver_time.size() <= frame)
					return -1.0;
				return solver_time[frame] / solver_calls[frame];
			}

			void obligation_done(size_t frame)
			{
				total_obligations_handled++;

				while (obligations_handled.size() <= frame)
					obligations_handled.push_back(0);

				obligations_handled[frame]++;
			}

			void obligation_done(size_t frame, double time)
			{
				assert(time > 0.0);
				obligation_done(frame);
				total_obligations_time += time;

				while (obligations_time.size() <= frame)
					obligations_time.push_back(0.0);
				assert(obligations_handled.size() == obligations_time.size());

				obligations_time[frame] += time;
			}

			double avg_obligation_time(size_t frame) const
			{
				if (obligations_time.size() <= frame)
					return -1.0;
				return obligations_time[frame] / obligations_handled[frame];
			}

			void subsumed(size_t frame, unsigned amount = 1)
			{
				total_subsumed_cubes += amount;
				while (subsumed_cubes.size() <= frame)
					subsumed_cubes.push_back(0);

				subsumed_cubes[frame] += amount;
			}

			std::string to_string() const 
			{
				std::stringstream ss;
				ss << *this << endl;
				return ss.str();
			}

			friend std::ostream& operator<<(std::ostream& out, const Statistics& s)
			{
				string frame_line("# - frame {level:<3} {name:<10}: {state:<20}");
				string frame_line_avg("# - frame {level:<3} {name:<10}: {state:<20} | avg: {avg}");
				out << "######################" << endl
					<< "# Statistics" << endl
					<< "######################" << endl;

				out << "# Solver" << endl
					<< "# - total time:  " << s.total_solver_time << endl
					<< "# - total calls: " << s.total_solver_calls << endl;
				for (size_t i = 0; i < s.solver_time.size(); i++)
					out << format(frame_line_avg, 
							arg("level", i), arg("name", "time"), arg("state", s.solver_time[i]), 
							arg("avg", s.avg_solver_time(i))) << endl;
				for (size_t i = 0; i < s.solver_calls.size(); i++)
					out << format(frame_line, 
							arg("level", i), arg("name", "calls"), arg("state", s.solver_calls[i])) << endl;
				out << "#" << endl;
				
				out << "# Obligations" << endl
					<< "# - total time:    " << s.total_obligations_time << endl
					<< "# - total handled: " << s.total_obligations_handled << endl;
				for (size_t i = 0; i < s.obligations_time.size(); i++)
					out << format(frame_line_avg, 
							arg("level", i), arg("name", "time"), arg("state", s.obligations_time[i]),
							arg("avg", s.avg_obligation_time(i))) << endl;
				for (size_t i = 0; i < s.obligations_handled.size(); i++)
					out << format(frame_line, 
							arg("level", i), arg("name", "handled"), arg("state", s.obligations_handled[i])) << endl;
				out << "#" << endl;
				
				out << "# Subsumed clauses" << endl
					<< "# - total: " << s.total_subsumed_cubes << endl;
				for (size_t i = 0; i < s.subsumed_cubes.size(); i++)
					out << format(frame_line, 
							arg("level", i), arg("name", "cubes"), arg("state", s.subsumed_cubes[i])) << endl;
				out << "#" << endl;

				out << "# Blocked clauses ignored" << endl
					<< "# - total: " << s.blocked_ignored << endl;

				return out << "######################" << endl;
			}
	};
}
#endif //STAT
