#ifndef PDR_RESULT_H
#define PDR_RESULT_H

#include "obligation.h"
#include "pdr-model.h"
#include "pebbling-model.h"
#include "tactic.h"
#include "z3-ext.h"

#include <cassert>
#include <fmt/format.h>
#include <memory>
#include <numeric>
#include <sstream>
#include <tabulate/row.hpp>
#include <tabulate/table.hpp>
#include <variant>
#include <vector>
#include <z3++.h>

namespace pdr
{
  // converts to bool: true there is an invariant, false if there is a trace
  // iterating walks through the linked list starting with trace
  struct PdrResult
  {
    using ResultRow = std::array<std::string, 3>;

    inline const static ResultRow fields = { "invariant index", "trace length",
      "total time" };

    struct Invariant
    {
      int level; // the F_i that gives the inductive invariant

      Invariant(int l);
      Invariant(std::optional<unsigned> c, int l);
    };

    struct Trace
    {
      using TraceState = std::vector<z3ext::LitStr>;
      using TraceVec   = std::vector<TraceState>;
      TraceVec states;
      unsigned length;
      unsigned n_marked;

      Trace();
      // Trace(Trace const& t) = default;
      Trace(unsigned l);
      Trace(std::shared_ptr<const PdrState> s);
      Trace(TraceVec const& trace_states);
      // Trace& operator=(Trace const&);
    };

    double time = 0.0;
    std::variant<Invariant, Trace> output;

    // Result builders
    static PdrResult found_trace(Trace::TraceVec const& s);
    static PdrResult found_trace(std::shared_ptr<PdrState> s);
    static PdrResult found_trace(PdrState&& s);
    static PdrResult incomplete_trace(unsigned length);
    static PdrResult found_invariant(int level);
    static PdrResult empty_true();
    static PdrResult empty_false();

    // building methods
    PdrResult& with_duration(double t);

    // for testing
    void append_final(const z3::expr_vector& f);

    operator bool() const;
    bool has_invariant() const;
    bool has_trace() const;

    // assumes that has_invariant() and has_trace() hold respectively
    const Invariant& invariant() const;
    const Trace& trace() const;
    Invariant& invariant();
    Trace& trace();

    void clean_trace();
    // lists { invariant level, trace length, time }
    ResultRow listing() const;
    tabulate::Table get_table() const;

   private:
    PdrResult(std::variant<Invariant, Trace> o);
    PdrResult(std::shared_ptr<PdrState> s);
    PdrResult(Trace::TraceVec const& trace_states);
    PdrResult(int level);
  };

  // collection of >= 1 pdr results that represents a single ipdr run
  // if decreasing: trace, trace, ..., invariant
  // if increasing: invariant, invariant, ..., trace
  class IpdrResult
  {
   public:
    IpdrResult(const IModel& m);
    IpdrResult(
        std::vector<std::string> const& v, std::vector<std::string> const& vp);
    virtual ~IpdrResult();

    void reset();

    // add a result to the aggregate
    // adds the row form process_result(r) to the pdr_summaries
    IpdrResult& add(const PdrResult& r);

    // output the pdr_summaries in a formatted table and append the total time
    // build using rows from process_row()
    tabulate::Table summary_table() const;
    // table with total header and single row
    // build using row from total_row()
    tabulate::Table total_table() const;

    double get_total_time() const;
    std::vector<double> g_times() const;

    // show a small string that described the end result of the run
    virtual std::string end_result() const           = 0;
    // represent total for a formatted table
    virtual tabulate::Table::Row_t total_row() const = 0;
    // get all traces using the virtual process_trace() function
    std::string all_traces() const;

   protected:
    // variables in the transition system, in current and next state.
    std::vector<std::string> vars;
    std::vector<std::string> vars_p;
    // accumulated time of every result
    double total_time{ 0.0 };
    // the pdr results that make up an ipdr result
    std::vector<PdrResult> original;
    // data extracted for the original pdr results
    std::vector<tabulate::Table::Row_t> pdr_summaries;
    // string representation of traces from each PdrResult
    std::vector<std::string> traces;

    // field names for table headers
    virtual const tabulate::Table::Row_t summary_header() const;
    virtual const tabulate::Table::Row_t total_header() const = 0;

    // records result in "originals" and adds it to the total_time
    // returns a row for "pdr_summaries":
    //  { processes, max_proc, invariant level, trace length, time }
    // called by add(PdrResult)
    const tabulate::Table::Row_t process_result(const PdrResult& r);
    // string representation of the trace or invariant
    virtual std::string process_trace(PdrResult const& res) const;
  };

  namespace result
  {
    std::string trace_table(PdrResult const& res,
        std::vector<std::string> const& vars,
        std::vector<std::string> const& vars_p);
  }

  namespace state
  {
    size_t n_marked(PdrResult::Trace::TraceState const& s);

    std::vector<std::string> marking(PdrResult::Trace::TraceState const& s,
        std::vector<std::string> header, unsigned width);
  } // namespace state

} // namespace pdr
#endif // PDR_RESULT_H
