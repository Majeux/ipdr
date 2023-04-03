#include "pdr-context.h"
#include "tactic.h"
#include "types-ext.h"

#include <iostream>
#include <memory>
#include <variant>

#define MIC_RETRIES_DEFAULT UINT_MAX
#define CTG_MAX_DEPTH_DEFAULT 1
#define CTG_MAX_COUNTERS_DEFAULT 5
#define SUBSUMED_CUT_DEFEAULT 0.5

namespace pdr
{

  Context::Context(z3::context& c, my::cli::ArgumentList const& args)
      : z3_ctx(c),
        type(Tactic::undef),
        mic_retries(args.mic_retries.value_or(MIC_RETRIES_DEFAULT)),
        subsumed_cutoff(args.subsumed_cutoff.value_or(SUBSUMED_CUT_DEFEAULT)),
        ctg_max_depth(args.ctg_max_depth.value_or(CTG_MAX_DEPTH_DEFAULT)),
        ctg_max_counters(
            args.ctg_max_counters.value_or(CTG_MAX_COUNTERS_DEFAULT))
  {
    using namespace my::variant;
    z3_ctx.set("unsat_core", true);
    z3_ctx.set("sat.core.minimize", true);
    // z3_ctx.set("sat.core.minimize_partial", true);
    z3_ctx.set("model", true);

    // clang-format off
    seed = std::visit(visitor{ 
        [](bool r) -> unsigned
        {
          if (r)
            {
              srand(time(0));
              return rand();
            }
            return 0u;
        },
        [](unsigned s) -> unsigned { return s; } },
      args.r_seed);
    // clang-format on
    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::Context(
      z3::context& c, my::cli::ArgumentList const& args, unsigned s)
      : z3_ctx(c),
        seed(s),
        type(Tactic::undef),
        mic_retries(args.mic_retries.value_or(MIC_RETRIES_DEFAULT))
  {
    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::operator z3::context&() { return z3_ctx; }
  Context::operator const z3::context&() const { return z3_ctx; }

  z3::context& Context::operator()() { return z3_ctx; }
} // namespace pdr
