#include "cli-parse.h"
#include "pdr.h"
#include "peterson-result.h"
#include "peterson.h"
#include "types-ext.h"
#include <cassert>
#include <climits>

namespace pdr::peterson
{
  IPDR::IPDR(
      my::cli::ArgumentList const& args, Context c, Logger& l, PetersonModel& m)
      : vIPDR(mk_pdr(args, c, l, m)), ts(m), control_setting(args.control_run)
  {
    auto ipdr = my::variant::get_cref<my::cli::algo::t_IPDR>(args.algorithm);
    assert(ipdr->get().type == Tactic::relax);
    (void)ipdr;
  }

  IpdrPetersonResult IPDR::control_run(Tactic tactic, unsigned processes)
  {
    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Decrement not implemented.");
        break;
      case Tactic::relax: return relax(processes, true);
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::run(Tactic tactic, unsigned max_bound)
  {
    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Constrain not implemented.");
        break;
      case Tactic::relax: return relax(max_bound, control_setting);
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::relax(unsigned max_bound, bool control)
  {
    alg->logger.and_whisper(
        "! Proving peterson for {} processes.", ts.n_processes());

    unsigned bound = 0;
    basic_reset(bound);

    IpdrPetersonResult total(ts, Tactic::relax);

    pdr::PdrResult invariant = alg->run();
    total.add(invariant, bound);

    for (bound = bound + 1; invariant && bound <= max_bound; bound++)
    {
      spdlog::stopwatch timer;
      if (control)
        basic_reset(bound);
      else
        relax_reset(bound);
      total.append_inc_time(collect_inc_time(bound, timer.elapsed().count()));

      invariant = alg->run();

      total.add(invariant, bound);
    }

    if (invariant && bound > max_bound) // last run did not find a trace
    {
      alg->logger.and_whisper("! No trace exists.");
      return total;
    }
    // N is minimal
    alg->logger.and_whisper("! Counter for switches switch", bound);
    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned switches)
  {
    if (ts.get_switch_bound() != switches)
    {
      alg->logger.and_show("naive change from {} -> {} switches",
          ts.get_switch_bound().value_or(UINT_MAX), switches);
      ts.constrain_switches(switches);
    }
    else
    {
      alg->logger.and_show("switches kept at {}", switches);
    }

    alg->ctx.type = Tactic::basic;
    alg->reset();
  }

  void IPDR::relax_reset(unsigned switches)
  {
    unsigned old                   = ts.get_switch_bound().value();
    z3::expr_vector old_constraint = z3ext::copy(ts.get_constraint());
    assert(switches > old);

    alg->logger.and_show("increment from {} -> {} switches", old, switches);

    ts.constrain_switches(switches);
    alg->relax();
  }
} // namespace pdr::peterson
