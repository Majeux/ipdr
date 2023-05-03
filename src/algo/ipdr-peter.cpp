#include "cli-parse.h"
#include "pdr.h"
#include "peterson-result.h"
#include "peterson.h"
#include "types-ext.h"
#include <cassert>

namespace pdr::peterson
{
  IPDR::IPDR(
      my::cli::ArgumentList const& args, Context c, Logger& l, PetersonModel& m)
      : vIPDR(args, c, l, m), ts(m), control_setting(args.control_run)
  {
    auto ipdr = my::variant::get_cref<my::cli::algo::t_IPDR>(args.algorithm);
    assert(ipdr->get().type == Tactic::relax);
  }

  IpdrPetersonResult IPDR::control_run(Tactic tactic, unsigned processes)
  {
    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Decrement not implemented.");
        break;
      case Tactic::relax: return relax(processes, true);
      case Tactic::inc_jump_test: relax_jump_test(processes, 10); break;
      case Tactic::inc_one_test: relax_jump_test(processes, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::run(Tactic tactic, std::optional<unsigned> processes)
  {
    unsigned p;
    if (processes)
    {
      p = processes.value();
      basic_reset(p);
    }
    else
      p = ts.n_processes();

    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Constrain not implemented.");
        break;
      case Tactic::relax: return relax(p, control_setting);
      case Tactic::inc_jump_test: relax_jump_test(p, 10); break;
      case Tactic::inc_one_test: relax_jump_test(p, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::relax(unsigned p, bool control)
  {
    assert(p == ts.n_processes());
    unsigned N = ts.max_processes();
    alg.logger.and_whisper("! Proving peterson for {}..{} processes.", p, N);

    IpdrPetersonResult total(ts, Tactic::relax);

    pdr::PdrResult invariant = alg.run();
    total.add(invariant, ts.n_processes());

    for (p = p + 1; invariant && p <= N; p++)
    {
      spdlog::stopwatch timer;
      if (control)
        basic_reset(p);
      else
        relax_reset(p);
      total.append_inc_time(timer.elapsed().count()); // adds to previous result

      invariant = alg.run();

      total.add(invariant, ts.n_processes());
    }

    if (invariant && p > N) // last run did not find a trace
    {
      alg.logger.and_whisper("! No trace exists.");
      return total;
    }
    // N is minimal
    alg.logger.and_whisper("! Counter for p={}", p - 1);
    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned processes)
  {
    assert(std::addressof(ts) == std::addressof(alg.ts));

    unsigned old = ts.n_processes();

    alg.logger.and_show("naive change from {} / {} -> {} / {}", old,
        ts.max_processes(), processes, ts.max_processes());

    ts.constrain(processes);
    alg.ctx.type = Tactic::basic;
    alg.reset();
  }

  void IPDR::relax_reset(unsigned processes)
  {
    assert(std::addressof(ts) == std::addressof(alg.ts));

    unsigned old = ts.n_processes();
    assert(processes > old);

    alg.logger.and_show("increment from {} / {} -> {} / {} processes", old,
        ts.max_processes(), processes, ts.max_processes());

    ts.constrain(processes);

    alg.ctx.type = Tactic::relax;
    alg.frames.copy_to_F1();
  }

  IpdrPetersonResult IPDR::relax_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);

    IpdrPetersonResult total(ts, Tactic::relax);
    basic_reset(start);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant, ts.n_processes());

    unsigned oldp = ts.n_processes();
    unsigned newp = oldp + step;
    assert(newp > 0);
    assert(oldp < newp);
    assert(newp <= ts.max_processes());

    relax_reset(newp);
    invariant = alg.run();

    total.add(invariant, ts.n_processes());

    return total;
  }
} // namespace pdr::peterson
