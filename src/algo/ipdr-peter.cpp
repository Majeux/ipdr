#include "pdr.h"
#include "peterson-result.h"
#include "peterson.h"
#include <cassert>

namespace pdr::peterson
{
  IPDR::IPDR(
      Context& c, PetersonModel& m, my::cli::ArgumentList args, Logger& l)
      : alg(c, m, l), model(m)
  {
  }

  PetersonResult IPDR::control_run(Tactic tactic, unsigned processes)
  {
    switch (tactic)
    {
      case Tactic::decrement:
        throw std::invalid_argument("Decrement not implemented.");
        break;
      case Tactic::increment: return relax(processes, true);
      case Tactic::inc_jump_test: relax_jump_test(processes, 10); break;
      case Tactic::inc_one_test: relax_jump_test(processes, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PetersonResult IPDR::run(Tactic tactic, unsigned processes, bool control)
  {
    switch (tactic)
    {
      case Tactic::decrement:
        throw std::invalid_argument("Decrement not implemented.");
        break;
      case Tactic::increment: return relax(processes, control);
      case Tactic::inc_jump_test: relax_jump_test(processes, 10); break;
      case Tactic::inc_one_test: relax_jump_test(processes, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PetersonResult IPDR::relax(unsigned processes, bool control)
  {
    unsigned p = processes, N = model.max_processes();
    alg.logger.and_whisper(
        "! Proving peterson for {}..{} processes.", processes, N);

    PetersonResult total(model, Tactic::increment);

    basic_reset(p);
    pdr::PdrResult invariant = alg.run();
    total << invariant;

    for (p = p + 1; invariant && p <= N; p++)
    {
      if (control)
        basic_reset(p);
      else
        relax_reset(p);

      invariant = alg.run();

      total << invariant;
    }

    if (p > N) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return total;
    }
    // N is minimal
    alg.logger.and_whisper("! Found optimum: {}.", p);
    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned processes)
  {
    assert(std::addressof(model) == std::addressof(alg.model));

    unsigned old = model.n_processes();

    alg.logger.and_show("naive change from {} / {}) -> {} / {}", old,
        model.max_processes(), processes, model.max_processes());

    model.constrain(processes);
    alg.ctx.type = Tactic::basic;
    alg.frames.reset();
  }

  void IPDR::relax_reset(unsigned processes)
  {
    assert(std::addressof(model) == std::addressof(alg.model));

    unsigned old = model.n_processes();
    assert(processes > old);

    alg.logger.and_show("increment from {} / {} -> {} / {} pebbles", old,
        model.max_processes(), processes, model.max_processes());

    model.constrain(processes);

    alg.ctx.type = Tactic::increment;
    alg.frames.reset_to_F1();
  }

  PetersonResult IPDR::relax_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);

    PetersonResult total(model, Tactic::increment);
    basic_reset(start);
    pdr::PdrResult invariant = alg.run();
    total << invariant;

    unsigned oldp = model.n_processes();
    unsigned newp = oldp + step;
    assert(newp > 0);
    assert(oldp < newp);
    assert(newp <= model.max_processes());

    relax_reset(newp);
    invariant = alg.run();

    total << invariant;

    return total;
  }

} // namespace pdr::peterson
