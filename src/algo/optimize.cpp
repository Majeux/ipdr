#include "pdr-model.h"
#include "pdr.h"
#include <bits/types/FILE.h>
#include <cassert>
#include <cstddef>
#include <string>

namespace pdr::pebbling
{
  using std::optional;

  Optimizer::Optimizer(Context& c, Model& m, Logger& l) : alg(c, m, l), model(m), latest_results(m) {}

  optional<unsigned> Optimizer::control_run(my::cli::ArgumentList args)
  {
    switch (args.tactic)
    {
      case pdr::Tactic::decrement: return decrement(true);
      case pdr::Tactic::increment: return increment(true);
      case pdr::Tactic::inc_jump_test:
        inc_jump_test(args.max_pebbles.value(), 10);
        break;
      case pdr::Tactic::inc_one_test:
        inc_jump_test(args.max_pebbles.value(), 1);
        break;
      default:
        throw std::invalid_argument(
            "No optimization pdr tactic has been selected.");
    }
    return {};
  }

  optional<unsigned> Optimizer::run(my::cli::ArgumentList args)
  {
    switch (args.tactic)
    {
      case pdr::Tactic::decrement: return decrement(args.experiment_control);
      case pdr::Tactic::increment: return increment(args.experiment_control);
      case pdr::Tactic::inc_jump_test:
        inc_jump_test(args.max_pebbles.value(), 10);
        break;
      case pdr::Tactic::inc_one_test:
        inc_jump_test(args.max_pebbles.value(), 1);
        break;
      default:
        throw std::invalid_argument(
            "No optimization pdr tactic has been selected.");
    }
    return {};
  }

  optional<unsigned> Optimizer::increment(bool control)
  {
    alg.logger.and_whisper("! Optimization run: decrement.");
    latest_results.reset();

    unsigned N = model.get_f_pebbles(); // need at least this many pebbles

    pdr::Result invariant = alg.run(Tactic::basic, N);
    latest_results << invariant;

    for (N = N + 1; invariant && N <= model.n_nodes(); N++)
    {
      if (control)
        invariant = alg.run(Tactic::basic, N);
      else
        invariant = alg.increment_run(N);
      latest_results << invariant;
    }

    if (N > model.n_nodes()) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return {};
    }
    // N is minimal
    alg.logger.and_whisper("! Found optimum: {}.", N);
    return N;
  }

  optional<unsigned> Optimizer::decrement(bool control)
  {
    alg.logger.and_whisper("! Optimization run: decrement.");
    latest_results.reset();

    unsigned N = model.n_nodes(); // need at least this many pebbles

    pdr::Result invariant = alg.run(Tactic::basic, N);
    latest_results << invariant;
    if (!invariant)
      N = std::min(N, invariant.trace().marked);

    for (N = N - 1; !invariant && N >= model.get_f_pebbles(); N--)
    {
      if (control)
        invariant = alg.run(Tactic::basic, N);
      else
        invariant = alg.decrement_run(N);
      latest_results << invariant;
      if (!invariant)
        N = std::min(N, invariant.trace().marked);
    }

    if (invariant) // last run did not find a trace
    {
      alg.logger.and_whisper("! No optimum exists.");
      return {};
    }
    // the previous N was optimal
    alg.logger.and_whisper("! Found optimum: {}.", N + 1);
    return N + 1;
  }

  void Optimizer::inc_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);
    pdr::Result invariant    = alg.run(Tactic::basic, start);
    latest_results << invariant;

    int maxp = alg.frames.max_pebbles.value();
    int newp = maxp + step;
    assert(newp > 0);
    assert(maxp < newp);
    if (newp <= (int)model.n_nodes())
    {
      invariant = alg.increment_run(newp);
      latest_results << invariant;
    }
  }

  void Optimizer::dump_solver(std::ofstream& out) const
  {
    alg.show_solver(out);
  }
} // namespace pdr::pebbling
