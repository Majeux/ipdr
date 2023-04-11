#include "pdr-context.h"
#include "cli-parse.h"
#include "tactic.h"
#include "types-ext.h"

#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <variant>

#define SKIP_BLOCKED_DEFAULT true
#define MIC_RETRIES_DEFAULT UINT_MAX
#define CTG_MAX_DEPTH_DEFAULT 1
#define CTG_MAX_COUNTERS_DEFAULT 3
#define SUBSUMED_CUT_DEFEAULT 0.5

namespace pdr
{

  void Context::init_settings(my::cli::ArgumentList const& args)
  {
    min_core         = false;
    part_min_core    = false;
    type             = Tactic::undef;
    skip_blocked     = args.skip_blocked.value_or(SKIP_BLOCKED_DEFAULT);
    mic_retries      = args.mic_retries.value_or(MIC_RETRIES_DEFAULT);
    subsumed_cutoff  = args.subsumed_cutoff.value_or(SUBSUMED_CUT_DEFEAULT);
    ctg_max_depth    = args.ctg_max_depth.value_or(CTG_MAX_DEPTH_DEFAULT);
    ctg_max_counters = args.ctg_max_counters.value_or(CTG_MAX_COUNTERS_DEFAULT);

    z3_ctx.set("unsat_core", true);
    z3_ctx.set("model", true);
    if (min_core)
      z3_ctx.set("sat.core.minimize", true);
    if (part_min_core)
      z3_ctx.set("sat.core.minimize_partial", true);

    if (min_core && part_min_core)
      throw std::invalid_argument(
          "cannot set both sat.core.minimize and sat.core.minimize_partial");
  }

  Context::Context(z3::context& c, my::cli::ArgumentList const& args)
      : z3_ctx(c)
  {
    init_settings(args);
    using namespace my::variant;
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
    std::cout << settings_str() << std::endl;
  }

  Context::Context(
      z3::context& c, my::cli::ArgumentList const& args, unsigned s)
      : z3_ctx(c)
  {
    init_settings(args);
    seed = s;
    std::cout << settings_str() << std::endl;
  }

  Context::operator z3::context&() { return z3_ctx; }
  Context::operator const z3::context&() const { return z3_ctx; }

  z3::context& Context::operator()() { return z3_ctx; }

  std::string Context::settings_str() const
  {
    using fmt::format;
    using std::endl;

    std::stringstream ss;

    ss << "-------------" << endl
       << "Context settings:" << endl
       << format("\tmin_core: {}", min_core) << endl
       << format("\tpart_min_core: {}", part_min_core) << endl
       << format("\tskip_blocked: {}", skip_blocked ? "true" : "false") << endl
       << format("\tmic_retries: {}", mic_retries) << endl
       << format("\tsubsumed_cutoff: {}", subsumed_cutoff) << endl
       << format("\tctg_max_depth: {}", ctg_max_depth) << endl
       << format("\tctg_max_counters: {}", ctg_max_counters) << endl
       << format("\tseed: {}", seed) << endl
       << "-------------";

    return ss.str();
  }
} // namespace pdr
