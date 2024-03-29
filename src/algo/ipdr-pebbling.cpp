#include "logger.h"
#include "pdr-model.h"
#include "pdr.h"
#include "result.h"
#include "types-ext.h"
#include "z3pdr.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/stopwatch.h>
#include <stdexcept>
#include <string>
#include <z3++.h>

namespace pdr::pebbling
{
  using std::optional;

  IPDR::IPDR(
      my::cli::ArgumentList const& args, Context c, Logger& l, PebblingModel& m)
      : vIPDR(mk_pdr(args, c, l, m), args),
        ts(m),
        starting_pebbles()
  {
    auto const& peb =
        my::variant::get_cref<my::cli::model_t::Pebbling>(args.model)->get();
    starting_pebbles = peb.max_pebbles;
  }

  IpdrPebblingResult IPDR::control_run(Tactic tactic)
  {
    switch (tactic)
    {
      case Tactic::constrain: return constrain(true);
      case Tactic::relax: return relax(true);
      case Tactic::binary_search: return binary(true);
      default: break;
    }
    throw std::invalid_argument("No ipdr tactic has been selected.");
  }

  IpdrPebblingResult IPDR::run(Tactic tactic)
  {
    switch (tactic)
    {
      case Tactic::constrain: return constrain(args.control_run);
      case Tactic::relax: return relax(args.control_run);
      case Tactic::binary_search: return binary(args.control_run);
      default: break;
    }
    throw std::invalid_argument("No ipdr tactic has been selected.");
  }

  IpdrPebblingResult IPDR::relax(bool control)
  {
    alg->logger.and_whisper("! IPDR run: increment max pebbles.");

    IpdrPebblingResult total(args, ts, Tactic::relax);

    // need at least this many pebbles
    unsigned N = starting_pebbles.value_or(ts.get_f_pebbles());

    // initial run, no constraining functionality yet
    basic_reset(N);
    pdr::PdrResult invariant = alg->run();
    total.add(invariant, ts.get_pebble_constraint());

    for (N = N + 1; invariant && N <= ts.n_nodes(); N++)
    {
      assert(N > ts.get_pebble_constraint()); // check for overflows

      { // timed
        spdlog::stopwatch timer;
        if (control)
          basic_reset(N);
        else
        {
          if (args.simple_relax)
            relax_reset(N);
          else
            relax_reset_constrained(N);
        }
        total.append_inc_time(collect_inc_time(N, timer.elapsed().count()));
      }

      invariant = alg->run();

      total.add(invariant, ts.get_pebble_constraint());
    }

    if (N > ts.n_nodes()) // last run did not find a trace
      alg->logger.and_whisper("! No optimum exists.");
    else
      alg->logger.and_whisper("! Found optimum: {}.", N); // N is minimal

    return total;
  }

  IpdrPebblingResult IPDR::constrain(bool control)
  {
    alg->logger.and_whisper("! IPDR run: decrement max pebbles.");

    IpdrPebblingResult total(args, ts, Tactic::constrain);
    // we can use at most this many pebbles
    unsigned N = starting_pebbles.value_or(ts.n_nodes());

    // initial run, no constraining functionality yet
    basic_reset(N);
    pdr::PdrResult invariant = alg->run();
    total.add(invariant, ts.get_pebble_constraint());

    // found strategy may already use fewer pebbles than N
    if (invariant.has_trace())
    {
      assert(invariant.trace().n_marked <= N);
      N = invariant.trace().n_marked;
    }

    // We need to at least pebble the final state,
    // so iterate until a strategy is found or until then
    for (N = N - 1; !invariant && N >= ts.get_f_pebbles(); N--)
    {
      assert(N < ts.get_pebble_constraint());

      { // timed
        spdlog::stopwatch timer;
        if (control)
        {
          basic_reset(N);
          // adds to previous result
          total.append_inc_time(collect_inc_time(N, timer.elapsed().count()));
          invariant = alg->run();
        }
        else
        {
          optional<size_t> inv_frame = constrain_reset(N);
          // adds to previous result
          total.append_inc_time(collect_inc_time(N, timer.elapsed().count()));

          if (inv_frame)
            invariant = PdrResult::found_invariant(*inv_frame);
          else
            invariant = alg->run();
        }
      }

      total.add(invariant, ts.get_pebble_constraint());
      if (!invariant)
      {
        assert(invariant.trace().n_marked <= N);
        N = invariant.trace().n_marked;
      }
    }

    if (N < ts.get_f_pebbles() && !invariant)
    {
      alg->logger.and_whisper("Last trace has minimum possible cardinality.");
      total.add(PdrResult::empty_true(), ts.get_f_pebbles());
    }
    else
    {
      if (invariant) // last run did not find a trace
        alg->logger.and_whisper("! No optimum exists.");
      else // previous run is optimal trace
        alg->logger.and_whisper("! Found optimum: {}.", N + 1);
    }

    return total;
  }

  IpdrPebblingResult IPDR::binary(bool control)
  {
    alg->logger.and_whisper("! IPDR run: binary search exploring max pebbles.");

    IpdrPebblingResult total(args, ts, Tactic::binary_search);
    // we can use at most this many pebbles
    unsigned top    = starting_pebbles.value_or(ts.n_nodes());
    // and at least this many pebbles
    unsigned bottom = ts.get_f_pebbles();

    // initial run, no constraining functionality yet
    basic_reset(top);
    pdr::PdrResult invariant = alg->run();
    total.add(invariant, ts.get_pebble_constraint());

    // found strategy may already use fewer pebbles than N
    if (invariant.has_trace())
    {
      assert(invariant.trace().n_marked <= top);
      top = invariant.trace().n_marked;
    }

    // track previous pebble constraint to determine whether to constrain or
    // relax
    unsigned m_prev = top;

    while (bottom <= top)
    {
      unsigned m = (top + bottom) / 2; // halfway between N and bottom
      MYLOG_DEBUG(
          alg->logger, "binary search step: {} --- {} --- {}", bottom, m, top);

      optional<size_t> early_inv; // contains level if an invariant is found
                                  // during incrementation

      { // timed
        spdlog::stopwatch timer;
        if (control)
        {
          basic_reset(m);
        }
        else
        {
          assert(m != m_prev);
          if (m < m_prev)
            early_inv = constrain_reset(m);
          else if (m > m_prev)
          {
            if (args.simple_relax)
              relax_reset(m);
            else
              relax_reset_constrained(m);
          }
        }
        total.append_inc_time(collect_inc_time(m, timer.elapsed().count()));
      }

      if (early_inv)
        invariant = PdrResult::found_invariant(*early_inv);
      else
        invariant = alg->run();

      total.add(invariant, ts.get_pebble_constraint());

      if (invariant)
      {
        bottom = m + 1;
        MYLOG_DEBUG(
            alg->logger, "invariant found, try higher: bottom <- {}", bottom);
      }
      else // trace found
      {
        assert(invariant.trace().n_marked <= m);
        top = invariant.trace().n_marked - 1;
        MYLOG_DEBUG(alg->logger,
            "trace of length {} found, try lower: top <- {}",
            invariant.trace().n_marked, top);
      }

      m_prev = m;
    }

    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned pebbles)
  {
    std::optional<unsigned> current = ts.get_pebble_constraint();
    std::string from = current ? std::to_string(*current) : "any";
    alg->logger.and_show("naive change from {} -> {} pebbles", from, pebbles);

    ts.constrain(pebbles);
    alg->ctx.type = Tactic::basic;
    alg->reset();
  }

  void IPDR::relax_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = ts.get_pebble_constraint();
    assert(pebbles > old.value());
    alg->logger.and_show(
        "increment from {} -> {} pebbles", old.value(), pebbles);

    ts.constrain(pebbles);
    alg->relax();
  }

  void IPDR::relax_reset_constrained(unsigned pebbles)
  {
    using fmt::format;

    unsigned old                   = ts.get_pebble_constraint().value();
    z3::expr_vector old_constraint = z3ext::copy(ts.get_constraint());
    assert(pebbles > old);
    assert(old_constraint.size() == 2);

    alg->logger.and_show("increment from {} -> {} pebbles", old, pebbles);

    ts.constrain(pebbles);
    alg->ctx.type = Tactic::relax;

    try
    {
      auto myalg = dynamic_cast<PDR&>(*alg);
      myalg.frames.copy_to_Fk_keep(old, old_constraint);
    }
    catch (...)
    {
      throw std::runtime_error("(experimental) constrained copy for PDR only.");
    }
  }

  std::optional<size_t> IPDR::constrain_reset(unsigned pebbles)
  {
    using fmt::format;

    optional<unsigned> old = ts.get_pebble_constraint();
    assert(pebbles < old.value());
    alg->logger.and_show(
        "decrement from {} -> {} pebbles", old.value(), pebbles);

    ts.constrain(pebbles);
    return alg->constrain();
  }
} // namespace pdr::pebbling
