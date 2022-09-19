#ifndef PDR_RES
#define PDR_RES

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
    using ResultRow = std::array<std::string, 5>;
    class const_iterator;

    struct Invariant
    {
      int level; // the F_i that gives the inductive invariant

      Invariant(int l = 1);
      Invariant(std::optional<unsigned> c, int l = 1);
    };

    struct Trace
    {
      std::vector<z3::expr_vector> states;
      unsigned length;

      Trace();
      Trace(unsigned l);
      Trace(std::shared_ptr<const State> s);
    };

    std::optional<unsigned> constraint;
    double time = 0.0;
    std::variant<Invariant, Trace> output;

    // Result builders
    static PdrResult found_trace(
        std::optional<unsigned> constraint, std::shared_ptr<State> s);
    static PdrResult found_trace(std::optional<unsigned> constraint, State&& s);
    static PdrResult found_invariant(
        std::optional<unsigned> constraint, int level);
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
    std::string_view string_rep() const;

    void clean_trace();
    ResultRow listing() const;
    void process(const pebbling::PebblingModel& model);
    // iterators over the Trace. empty if there is an Invariant
    const_iterator begin();
    const_iterator end();

   private:
    bool finalized  = false;
    std::string str = "";

    PdrResult(std::optional<unsigned> constr, std::shared_ptr<State> s);
    PdrResult(std::optional<unsigned> constr, int l);
  };

  // collection of >= 1 pdr results that represents a single ipdr run
  // if decreasing: trace, trace, ..., invariant
  // if increasing: invariant, invariant, ..., trace
  class IpdrResult
  {
   public:
    virtual ~IpdrResult();
    tabulate::Table new_table() const;
    void reset();
    virtual void show(std::ostream& out) const;
    void show_traces(std::ostream& out) const;
    IpdrResult& add(PdrResult& r);
    tabulate::Table raw_table() const;
    std::vector<double> g_times() const;
    friend IpdrResult& operator<<(IpdrResult& rs, PdrResult& r);

   protected:
    std::vector<tabulate::Table::Row_t> rows;

    std::vector<PdrResult> original;
    std::vector<std::string> traces;

    virtual const tabulate::Table::Row_t header() const;

    friend class ExperimentResults;
  };

  namespace pebbling
  {
    // aggregates multiple pdr runs into a single ipdr result for pebbling
    // collects: total time spent, highest level invariant, and trace with the
    // lowest marking
    class PebblingResult final : public IpdrResult
    {
     public:
      struct PebblingInvariant
      {
        PdrResult::Invariant invariant;
        std::optional<unsigned> constraint;
      };
      struct PebblingTrace
      {
        PdrResult::Trace trace;
        unsigned pebbled{0};
      };
      struct Data_t
      {
        double time{0.0};
        std::optional<PebblingInvariant> inv;
        std::optional<PebblingTrace> strategy;
      };

      PebblingResult(const pebbling::PebblingModel& m, Tactic t);
      PebblingResult(const IpdrResult& r, const PebblingModel& m, Tactic t);

      void add_to_table(tabulate::Table& t) const;
      void show(std::ostream& out) const override;
      void show_raw(std::ostream& out) const;
      PebblingResult& add(PdrResult& r);
      friend PebblingResult& operator<<(PebblingResult& rs, PdrResult& r);

     private:
      const PebblingModel& model;
      const Tactic tactic;
      Data_t total; // total time, max invariant, min trace
      unsigned invariants{0};
      unsigned traces{0};

      // aggregate result into total
      void acc_update(const PdrResult& r);
      const tabulate::Table::Row_t header() const override;
    };
  } // namespace pebbling
} // namespace pdr
#endif // PDR_RES
