#ifndef BOUNDED_H
#define BOUNDED_H

#include "cli-parse.h"
#include "dag.h"
#include "pebbling-result.h"
#include "result.h"
#include <cstddef>
#include <optional>
#include <spdlog/stopwatch.h>
#include <tabulate/table.hpp>
#include <z3++.h>

namespace bounded
{
  // clang-format off
  struct Literal
  {
    size_t index;
    enum polarity {positive, negative} sign;
    size_t time;

    operator bool() const { return sign == polarity::positive; }
  };
  // clang-format on

  struct ConstrainedExpr
  {
    z3::expr expression;
    z3::expr constraint;

    operator z3::expr_vector()
    {
      z3::expr_vector rv(expression.ctx());
      rv.push_back(expression);
      rv.push_back(constraint);
      return rv;
    }
  };

  struct Marking
  {
    std::string name;
    size_t timestep;
    bool mark;
  };

  struct TraceRow
  {
    unsigned marked;
    std::vector<std::string> states;

    operator const std::vector<std::string>&() const { return states; }
    TraceRow(size_t size, std::string initial)
        : marked(0), states(size, initial)
    {
    }

    void mark(size_t i, const Marking& l, std::string_view fill)
    {
      states.at(i) = l.mark ? fill : "";
      if (l.mark)
        marked++;
    }

    // std::string& at(size_t i) { return states.at(i); }
    // const std::string& at(size_t i) const { return states.at(i); }

    std::vector<std::string>::const_iterator begin() const
    {
      return states.begin();
    }
    std::vector<std::string>::const_iterator end() const
    {
      return states.end();
    }
  };

  class BoundedPebbling
  {
   public:
    // using Literals = std::vector<Literal>;
    using Literals = z3::expr_vector;
    using TraceState = pdr::PdrResult::Trace::TraceState;
    using TraceVec = pdr::PdrResult::Trace::TraceVec;

    std::vector<Literals> lits_at_time;

    BoundedPebbling(const dag::Graph& G, my::cli::ArgumentList const& args);

    pdr::pebbling::IpdrPebblingResult run();

   private:
    z3::context context;
    const dag::Graph& graph;
    z3::solver solver;

    const int time_limit{ 120 };
    const double dtime_limit{ (double)time_limit };

    std::vector<std::string> lit_names;
    const size_t n_lits;

    // constraint presently enforced in the solver
    std::optional<size_t> cardinality;
    // the amount of steps that are added to the solver
    // the last transition is from `current_bound-1` to `current_bound`
    std::optional<size_t> current_bound;

    std::optional<TraceVec> trace;

    spdlog::stopwatch timer;
    spdlog::stopwatch card_timer;
    spdlog::stopwatch step_timer;
    double total_time;
    std::vector<double> sub_times;

    std::ofstream result_out;

    void reset();
    z3::expr lit(std::string_view name, size_t time_step);
    z3::expr constraint(const z3::expr_vector& lits);
    // empty state and cardinality clase (index 0)
    ConstrainedExpr initial();
    // final state for the last state and cardinality
    ConstrainedExpr final();
    // returns the transition for step i -> i+1
    // with the cardinality clause for step i+1
    ConstrainedExpr trans_step(size_t i);
    // transition relations for an amount of steps
    void push_transitions(size_t steps);

    z3::check_result check(size_t steps, double allowance);

    pdr::PdrResult::Trace::TraceVec get_trace(size_t length) const;
    std::string strategy_table(const std::vector<TraceRow>& content) const;
    void store_strategy(size_t length);
    void dump_times(std::ostream& out) const;

  }; // class Bounded
} // namespace bounded

#endif // BOUNDED_H
