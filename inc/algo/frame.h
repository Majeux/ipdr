#ifndef FRAME
#define FRAME

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
    class Frame
    {
      private:
        z3ext::CubeSet blocked_cubes;
        unsigned level;
        Logger& logger;
        // the arguments of the clause are sorted by mic, use id to search

        void init_solver();

      public:
        Frame(unsigned i, Logger& l);

        unsigned remove_subsumed(const z3::expr_vector& cube, bool remove_equal);
        bool blocked(const z3::expr_vector& cube);
        bool block(const z3::expr_vector& cube);

        // Frame comparisons
        bool equals(const Frame& f) const;
        std::vector<z3::expr_vector> diff(const Frame& f) const;

        // getters
        const z3ext::CubeSet& get_blocked() const;
        bool empty() const;

        // string representations
        std::string blocked_str() const;
    };
} // namespace pdr

#endif // FRAME
