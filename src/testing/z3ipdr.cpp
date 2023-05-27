#include "pdr-model.h"
#include "pebbling-result.h"
#include "types-ext.h"
#include "z3pdr.h"

#include <cassert>
#include <optional>

namespace pdr::test
{
  //using std::optional;

  //z3PebblingIPDR::z3PebblingIPDR(my::cli::ArgumentList const& args, Context& c,
  //    Logger& l, Z3PebblingModel& m)
  //    : alg(c, l, m), ts(m), starting_pebbles()
  //{
  //  assert(std::addressof(ts) == std::addressof(alg.ts));
  //  auto const& peb =
  //      my::variant::get_cref<my::cli::model_t::Pebbling>(args.model)->get();
  //  starting_pebbles = peb.max_pebbles;
  //}

  //pebbling::IpdrPebblingResult z3PebblingIPDR::control_run(Tactic tactic)
  //{
  //  switch (tactic)
  //  {
  //    case Tactic::constrain: return constrain(true);
  //    case Tactic::relax: return relax(true);
  //    default:
  //      throw std::invalid_argument(
  //          "No supported optimization ipdr tactic has been selected.");
  //  }
  //}

  //pebbling::IpdrPebblingResult z3PebblingIPDR::relax(bool control)
  //{
  //  assert(control && "only naive ipdr supported for z3");
  //  alg.logger.and_whisper("! Optimization run: increment max pebbles.");

  //  unsigned n_nodes                   = ts.dag.nodes.size();
  //  unsigned final_n_pebbles           = ts.dag.output.size();
  //  pebbling::IpdrPebblingResult total = new_total(Tactic::relax);

  //  // need at least this many pebbles
  //  unsigned N = starting_pebbles.value_or(final_n_pebbles);

  //  basic_reset(N);
  //  pdr::PdrResult invariant = alg.run();
  //  total.add(invariant, ts.get_pebble_constraint());

  //  for (N = N + 1; invariant && N <= n_nodes; N++)
  //  {
  //    assert(N > ts.get_pebble_constraint());
  //    basic_reset(N);

  //    invariant = alg.run();

  //    total.add(invariant, ts.get_pebble_constraint());
  //  }

  //  if (N > n_nodes) // last run did not find a trace
  //  {
  //    alg.logger.and_whisper("! No optimum exists.");
  //    return total;
  //  }
  //  // N is minimal
  //  alg.logger.and_whisper("! Found optimum: {}.", N);
  //  return total;
  //}

  //pebbling::IpdrPebblingResult z3PebblingIPDR::constrain(bool control)
  //{
  //  assert(control && "only naive ipdr supported for z3");
  //  alg.logger.and_whisper("! Optimization run: decrement max pebbles.");

  //  unsigned n_nodes                   = ts.dag.nodes.size();
  //  unsigned final_n_pebbles           = ts.dag.output.size();
  //  pebbling::IpdrPebblingResult total = new_total(Tactic::constrain);

  //  // we can use at most this many pebbles
  //  unsigned N = starting_pebbles.value_or(n_nodes);

  //  basic_reset(N);
  //  pdr::PdrResult invariant = alg.run();
  //  total.add(invariant, ts.get_pebble_constraint());
  //  if (!invariant)
  //    N = std::min(N, total.min_pebbles().value_or(N));

  //  // We need to at least pebble the final state,
  //  // so iterate until a strategy is found or until then
  //  for (N = N - 1; !invariant && N >= final_n_pebbles; N--)
  //  {
  //    assert(N < ts.get_pebble_constraint());
  //    basic_reset(N);
  //    invariant = alg.run();

  //    total.add(invariant, ts.get_pebble_constraint());
  //    if (!invariant)
  //      N = std::min(N, total.min_pebbles().value_or(N));
  //  }

  //  if (invariant) // last run did not find a trace
  //  {
  //    alg.logger.and_whisper("! No optimum exists.");
  //    return total;
  //  }
  //  // the previous N was optimal
  //  alg.logger.and_whisper("! Found optimum: {}.", N + 1);
  //  return total;
  //}

  //// Private members
  ////
  //pebbling::IpdrPebblingResult z3PebblingIPDR::new_total(Tactic t) const
  //{
  //  return pebbling::IpdrPebblingResult(ts.vars, ts.dag.output.size(), t);
  //}

  //void z3PebblingIPDR::basic_reset(unsigned pebbles)
  //{
  //  assert(std::addressof(ts) == std::addressof(alg.ts));

  //  std::optional<unsigned> current = ts.get_pebble_constraint();
  //  std::string from = current ? std::to_string(*current) : "any";
  //  alg.logger.and_show("naive change from {} -> {} pebbles", from, pebbles);

  //  ts.constrain(pebbles);
  //  alg.ctx.type = Tactic::basic;
  //  alg.reset();
  //}

  //z3PDR const& z3PebblingIPDR::internal_alg() const { return alg; }
} // namespace pdr::test
