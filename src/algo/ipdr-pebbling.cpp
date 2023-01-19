#include "pdr-model.h"
#include "pdr.h"
#include "types-ext.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace pdr::pebbling
{
  using std::optional;

  IPDR::IPDR(Context& c, PebblingModel& m, my::cli::ArgumentList const& args,
      Logger& l)
      : alg(c, l, m), ts(m), starting_pebbles()
  {
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));
    auto const& peb =
        my::variant::get_cref<my::cli::model_t::Pebbling>(args.model)->get();
    starting_pebbles = peb.max_pebbles;
  }

  PebblingResult IPDR::control_run(Tactic tactic)
  {
    switch (tactic)
    {
      case Tactic::constrain: return constrain(true);
      case Tactic::relax: return relax(true);
      case Tactic::inc_jump_test:
        relax_jump_test(starting_pebbles.value(), 10);
        break;
      case Tactic::inc_one_test:
        relax_jump_test(starting_pebbles.value(), 1);
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
        relax_jump_test(starting_pebbles.value(), 10);
        break;
      case Tactic::inc_one_test:
        relax_jump_test(starting_pebbles.value(), 1);
        break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  PebblingResult IPDR::relax(bool control)
  {
    alg.logger.and_whisper("! Optimization run: increment max pebbles.");

    PebblingResult total(ts, Tactic::relax);
    // need at least this many pebbles
    unsigned N = starting_pebbles.value_or(ts.get_f_pebbles());

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);

    for (N = N + 1; invariant && N <= ts.n_nodes(); N++)
    {
      if (control)
        basic_reset(N);
      else
        relax_reset(N);

      invariant = alg.run();

      total.add(invariant);
    }

    if (N > ts.n_nodes()) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return total;
    }
    // N is minimal
    alg.logger.and_whisper("! Found optimum: {}.", N);
    return total;
  }

  PebblingResult IPDR::constrain(bool control)
  {
    alg.logger.and_whisper("! Optimization run: decrement max pebbles.");

    PebblingResult total(ts, Tactic::constrain);
    // we can use at most this many pebbles
    unsigned N = starting_pebbles.value_or(ts.n_nodes());

    basic_reset(N);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);
    if (!invariant)
      N = std::min(N, total.min_pebbles().value_or(N));

    // We need to at least pebble the final state,
    // so iterate until a strategy is found or until then
    for (N = N - 1; !invariant && N >= ts.get_f_pebbles(); N--)
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
      alg.logger.and_whisper("! No optimum exists.");
      return total;
    }
    // the previous N was optimal
    alg.logger.and_whisper("! Found optimum: {}.", N + 1);
    return total;
  }

  PebblingResult IPDR::relax_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);

    PebblingResult total(ts, Tactic::relax);
    basic_reset(start);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant);

    unsigned maxp = ts.get_pebble_constraint().value();
    unsigned newp = maxp + step;
    assert(newp > 0);
    assert(maxp < newp);
    if (newp <= ts.n_nodes())
    {
      relax_reset(newp);
      invariant = alg.run();

      total.add(invariant);
    }

    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned pebbles)
  {
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));

    std::optional<unsigned> current = ts.get_pebble_constraint();
    std::string from = current ? std::to_string(*current) : "any";
    alg.logger.and_show("naive change from {} -> {} pebbles", from, pebbles);

    ts.constrain(pebbles);
    alg.ctx.type = Tactic::basic;
    alg.reset();
  }

  void IPDR::relax_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = ts.get_pebble_constraint();
    assert(pebbles > old.value());
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));
    alg.logger.and_show(
        "increment from {} -> {} pebbles", old.value(), pebbles);

    ts.constrain(pebbles);

    alg.ctx.type = Tactic::relax;
    alg.frames.reset_to_F1();
  }

  std::optional<size_t> IPDR::constrain_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = ts.get_pebble_constraint();
    assert(pebbles < old.value());
    assert(std::addressof(model) == std::addressof(alg.ctx.ts));
    alg.logger.and_show(
        "decrement from {} -> {} pebbles", old.value(), pebbles);

    ts.constrain(pebbles);

    alg.ctx.type = Tactic::constrain;
    return alg.frames.reuse();
  }

  PDR const& IPDR::internal_alg() const { return alg; }
} // namespace pdr::pebbling
