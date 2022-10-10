#ifndef PDR_RESULT_H
#define PDR_RESULT_H

#include "TextTable.h"
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
      Trace(std::shared_ptr<const State> s);

      unsigned total_pebbled() const;
    };

    double time = 0.0;
    std::variant<Invariant, Trace> output;

    // Result builders
    static PdrResult found_trace(std::shared_ptr<State> s);
    static PdrResult found_trace(State&& s);
    static PdrResult found_invariant(int level);
    static PdrResult empty_true();
    static PdrResult empty_false();

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
    PdrResult(std::shared_ptr<State> s);
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
    // adds a pdr result to the IpdrResult::rows table
    virtual tabulate::Table::Row_t table_row(const PdrResult& r);

    friend IpdrResult& operator<<(IpdrResult& rs, const PdrResult& r);

   protected:
    // the pdr results that make up an ipdr result
    std::vector<PdrResult> original;
    // summary of the "original" vector
    std::vector<tabulate::Table::Row_t> rows;

    virtual const tabulate::Table::Row_t header() const;
    virtual std::string process_trace(const PdrResult& res) const;

   private:
    const pdr::IModel& model;
  };
} // namespace pdr
#endif // PDR_RESULT_H
