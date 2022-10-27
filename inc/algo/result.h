#ifndef PDR_RESULT_H
#define PDR_RESULT_H

#include "obligation.h"
#include "output.h"
#include "pdr-model.h"
#include "pebbling-model.h"
#include "tactic.h"

#include <cassert>
#include <fmt/format.h>
#include <memory>
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

    inline const static ResultRow header = { "invariant index", "trace length",
      "Total time" };

    struct Invariant
    {
      int level; // the F_i that gives the inductive invariant

      Invariant(int l);
      Invariant(std::optional<unsigned> c, int l);
    };

    struct Trace
    {
      std::vector<z3::expr_vector> states;
      unsigned length;

      Trace();
      Trace(unsigned l);
      Trace(std::shared_ptr<const PdrState> s);

      unsigned total_pebbled() const;
    };

    double time = 0.0;
    std::variant<Invariant, Trace> output;

    // Result builders
    static PdrResult found_trace(std::shared_ptr<PdrState> s);
    static PdrResult found_trace(PdrState&& s);
    static PdrResult found_invariant(int level);
    static PdrResult empty_true();
    static PdrResult empty_false();

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

   private:
    PdrResult(std::shared_ptr<PdrState> s);
    PdrResult(int l);
  };

  // collection of >= 1 pdr results that represents a single ipdr run
  // if decreasing: trace, trace, ..., invariant
  // if increasing: invariant, invariant, ..., trace
  class IpdrResult
  {
   public:
    IpdrResult(const IModel& m);
    virtual ~IpdrResult();
    tabulate::Table new_table() const;
    void reset();
    tabulate::Table raw_table() const;
    void show_traces(std::ostream& out) const;
    std::vector<double> g_times() const;

    virtual void show(std::ostream& out) const;
    IpdrResult& add(const PdrResult& r);

    friend IpdrResult& operator<<(IpdrResult& rs, const PdrResult& r);

   protected:
    // the pdr results that make up an ipdr result
    std::vector<PdrResult> original;
    // summary of the "original" vector
    std::vector<tabulate::Table::Row_t> rows;

    // field names for internal and summary tables
    virtual const tabulate::Table::Row_t header() const;
    virtual const tabulate::Table::Row_t summary_header() const;

    // adds a pdr result to the IpdrResult::rows table
    // { processes, max_proc, invariant level, trace length, time }
    // called by add(PdrResult)
    virtual const tabulate::Table::Row_t table_row(const PdrResult& r);

    // string representation of the trace or invariant
    virtual std::string process_trace(const PdrResult& res) const;

   private:
    const pdr::IModel& model;
  };
} // namespace pdr
#endif // PDR_RESULT_H
