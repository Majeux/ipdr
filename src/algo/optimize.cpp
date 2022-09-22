#include "pdr-model.h"
#include "pdr.h"
#include <bits/types/FILE.h>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace pdr::pebbling
{
  using std::optional;

  IPDR::IPDR(
      Context& c, PebblingModel& m, my::cli::ArgumentList args, Logger& l)
      : alg(c, m, l), model(m), tactic(args.tactic),
        starting_value(args.max_pebbles)
  {
  }

  PebblingResult IPDR::control_run()
  {
    switch (tactic)
    {
      case pdr::Tactic::decrement: return decrement(true);
      case pdr::Tactic::increment: return increment(true);
      case pdr::Tactic::inc_jump_test:
        inc_jump_test(starting_value.value(), 10);
        break;
      case pdr::Tactic::inc_one_test:
        inc_jump_test(starting_value.value(), 1);
        break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PebblingResult IPDR::run(bool control)
  {
    switch (tactic)
    {
      case pdr::Tactic::decrement: return decrement(control);
      case pdr::Tactic::increment: return increment(control);
      case pdr::Tactic::inc_jump_test:
        inc_jump_test(starting_value.value(), 10);
        break;
      case pdr::Tactic::inc_one_test:
        inc_jump_test(starting_value.value(), 1);
        break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PebblingResult IPDR::increment(bool control)
  {
    alg.logger.and_whisper("! Optimization run: increment max pebbles.");

    PebblingResult total(model, tactic);
    unsigned N = model.get_f_pebbles(); // need at least this many pebbles

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total << invariant;

    for (N = N + 1; invariant && N <= model.n_nodes(); N++)
    {
      if (control)
        basic_reset(N);
      else
        increment_reset(N);

      invariant = alg.run();

      total << invariant;
    }

    if (N > model.n_nodes()) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return total;
    }
    // N is minimal
    alg.logger.and_whisper("! Found optimum: {}.", N);
    return total;
  }

  PebblingResult IPDR::decrement(bool control)
  {
    alg.logger.and_whisper("! Optimization run: decrement max pebbles.");

    PebblingResult total(model, tactic);
    unsigned N = model.n_nodes(); // need at least this many pebbles

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total << invariant;
    if (!invariant)
      N = std::min(N, total.min_pebbles().value_or(N));

    for (N = N - 1; !invariant && N >= model.get_f_pebbles(); N--)
    {
      if (control)
      {
        basic_reset(N);
        invariant = alg.run();
      }
      else
      {
        optional<size_t> inv_frame = decrement_reset(N);
        if (inv_frame)
          invariant = PdrResult::found_invariant(*inv_frame);
        else
          invariant = alg.run();
      }

      total << invariant;
      if (!invariant)
        N = std::min(N, total.min_pebbles().value_or(N));
    }

    if (invariant) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return total;
    }
    // the previous N was optimal
    alg.logger.and_whisper("! Found optimum: {}.", N + 1);
    return total;
  }

  void IPDR::inc_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);
    pdr::PdrResult invariant = alg.run(Tactic::basic, start);
    total << invariant;

    int maxp = alg.frames.max_pebbles.value();
    int newp = maxp + step;
    assert(newp > 0);
    assert(maxp < newp);
    if (newp <= (int)model.n_nodes())
    {
      invariant = alg.increment_run(newp);

      invariant.process(model);
      total << invariant;
    }
  }

  void IPDR::dump_solver(std::ofstream& out) const { alg.show_solver(out); }

  // Private members
  //
  void IPDR::basic_reset(unsigned pebbles)
  {
    assert(std::addressof(model) == std::addressof(alg.model));

    alg.logger.and_show("naive change from {} -> {} pebbles",
        model.get_max_pebbles().value(), pebbles);

    model.constrain(pebbles);
    alg.ctx.type = Tactic::basic;
    alg.frames.reset();
  }

  void IPDR::increment_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = model.get_max_pebbles();
    assert(pebbles > old.value());
    assert(std::addressof(model) == std::addressof(alg.model));
    alg.logger.and_show(
        "increment from {} -> {} pebbles", old.value(), pebbles);

    model.constrain(pebbles);

    alg.ctx.type = Tactic::increment;
    alg.frames.reset_to_F1();
  }

  std::optional<size_t> IPDR::decrement_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = model.get_max_pebbles();
    assert(pebbles < old.value());
    assert(std::addressof(model) == std::addressof(alg.model));
    alg.logger.and_show(
        "decrement from {} -> {} pebbles", old.value(), pebbles);

    model.constrain(pebbles);

    alg.ctx.type = Tactic::decrement;
    return alg.frames.reuse();
  }
} // namespace pdr::pebbling
