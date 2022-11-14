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
      : alg(c, m, l), model(m), starting_value(args.starting_value)
  {
  }

  PebblingResult IPDR::control_run(Tactic tactic)
  {
    switch (tactic)
    {
      case Tactic::constrain: return constrain(true);
      case Tactic::relax: return relax(true);
      case Tactic::inc_jump_test:
        relax_jump_test(starting_value.value(), 10);
        break;
      case Tactic::inc_one_test:
        relax_jump_test(starting_value.value(), 1);
        break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PebblingResult IPDR::run(Tactic tactic, bool control)
  {
    switch (tactic)
    {
      case Tactic::constrain: return constrain(control);
      case Tactic::relax: return relax(control);
      case Tactic::inc_jump_test:
        relax_jump_test(starting_value.value(), 10);
        break;
      case Tactic::inc_one_test:
        relax_jump_test(starting_value.value(), 1);
        break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PebblingResult IPDR::relax(bool control)
  {
    alg.log.and_whisper("! Optimization run: increment max pebbles.");

    PebblingResult total(model, Tactic::relax);
    unsigned N = model.get_f_pebbles(); // need at least this many pebbles

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);

    for (N = N + 1; invariant && N <= model.n_nodes(); N++)
    {
      if (control)
        basic_reset(N);
      else
        relax_reset(N);

      invariant = alg.run();

      total.add(invariant);
    }

    if (N > model.n_nodes()) // last run did not find a trace
    {
      alg.log.and_whisper("! No optimum exists.");
      return total;
    }
    // N is minimal
    alg.log.and_whisper("! Found optimum: {}.", N);
    return total;
  }

  PebblingResult IPDR::constrain(bool control)
  {
    alg.log.and_whisper("! Optimization run: decrement max pebbles.");

    PebblingResult total(model, Tactic::constrain);
    unsigned N = model.n_nodes(); // need at least this many pebbles

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);
    if (!invariant)
      N = std::min(N, total.min_pebbles().value_or(N));

    // We need to at least pebble the final state,
    // so iterate until a strategy is found or until then
    for (N = N - 1; !invariant && N >= model.get_f_pebbles(); N--)
    {
      if (control)
      {
        basic_reset(N);
        invariant = alg.run();
      }
      else
      {
        optional<size_t> inv_frame = constrain_reset(N);
        if (inv_frame)
          invariant = PdrResult::found_invariant(*inv_frame);
        else
          invariant = alg.run();
      }

      total.add(invariant);
      if (!invariant)
        N = std::min(N, total.min_pebbles().value_or(N));
    }

    if (invariant) // last run did not find a trace
    {
      alg.log.and_whisper("! No optimum exists.");
      return total;
    }
    // the previous N was optimal
    alg.log.and_whisper("! Found optimum: {}.", N + 1);
    return total;
  }

  PebblingResult IPDR::relax_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.log.and_show("NEW INC JUMP TEST RUN");
    alg.log.and_show("start {}. step {}", start, step);

    PebblingResult total(model, Tactic::relax);
    basic_reset(start);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);

    unsigned maxp = model.get_max_pebbles().value();
    unsigned newp = maxp + step;
    assert(newp > 0);
    assert(maxp < newp);
    if (newp <= model.n_nodes())
    {
      relax_reset(newp);
      invariant = alg.run();

      total.add(invariant);
    }

    return total;
  }

  void IPDR::dump_solver(std::ofstream& out) const { alg.show_solver(out); }

  // Private members
  //
  void IPDR::basic_reset(unsigned pebbles)
  {
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));

    std::optional<unsigned> current = model.get_max_pebbles();
    std::string from = current ? std::to_string(*current) : "any";
    alg.log.and_show("naive change from {} -> {} pebbles",
        from, pebbles);

    model.constrain(pebbles);
    alg.ctx.type = Tactic::basic;
    alg.frames.reset();
  }

  void IPDR::relax_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = model.get_max_pebbles();
    assert(pebbles > old.value());
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));
    alg.log.and_show(
        "increment from {} -> {} pebbles", old.value(), pebbles);

    model.constrain(pebbles);

    alg.ctx.type = Tactic::relax;
    alg.frames.reset_to_F1();
  }

  std::optional<size_t> IPDR::constrain_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = model.get_max_pebbles();
    assert(pebbles < old.value());
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));
    alg.log.and_show(
        "decrement from {} -> {} pebbles", old.value(), pebbles);

    model.constrain(pebbles);

    alg.ctx.type = Tactic::constrain;
    return alg.frames.reuse();
  }
} // namespace pdr::pebbling
