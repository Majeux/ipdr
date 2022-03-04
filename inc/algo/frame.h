#ifndef FRAME
#define FRAME

#include "_logging.h"
#include "logger.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
    using CubeSet = std::set<z3::expr_vector, z3ext::expr_vector_less>;

    class Frame
    {
      private:
        CubeSet blocked_cubes;
        unsigned level;
        Logger& logger;
        std::unique_ptr<Solver> solver;
        // the arguments of the clause are sorted by mic, use id to search

        void init_solver();

      public:
        // Delta frame, without logger
        Frame(unsigned i, Logger& l);
        // Fat frame, with its own logger
        Frame(unsigned i, std::unique_ptr<Solver>&& s, Logger& l);

        void clean_solver();
        void set_stats(Statistics& s);

        unsigned remove_subsumed(const z3::expr_vector& cube);
        bool blocked(const z3::expr_vector& cube);
        bool block(const z3::expr_vector& cube);
        void block_in_solver(const z3::expr_vector& cube);

        // Frame comparisons
        bool equals(const Frame& f) const;
        std::vector<z3::expr_vector> diff(const Frame& f) const;

        // getters
        const CubeSet& get_blocked() const;
        bool empty() const;
        Solver& get_solver() const;
        const Solver& get_const_solver() const;

        // string representations
        std::string blocked_str() const;
    };
} // namespace pdr

#endif // FRAME
