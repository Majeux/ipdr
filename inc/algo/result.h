#ifndef PDR_RES
#define PDR_RES

#include "TextTable.h"
#include "obligation.h"
#include "output.h"
#include "pdr-model.h"
#include <cassert>
#include <fmt/format.h>
#include <memory>
#include <sstream>
#include <variant>
#include <vector>
namespace pdr
{
  // converts to bool: true if trace == null. if the property holds
  // iterating walks through the linked list starting with trace
  struct Result
  {
    using ResultRow = std::array<std::string, 5>;
    class const_iterator;

    struct Invariant
    {
      int level;
      Invariant(int l = 1);
    };

    struct Trace
    {
      std::shared_ptr<State> states_ll;
      unsigned length;
      unsigned marked;
      bool cleaned;

      Trace();
      Trace(std::shared_ptr<State> s);
    };

    std::optional<unsigned> constraint;
    double time = 0.0;
    std::variant<Invariant, Trace> output;

    // Result builders
    static Result found_trace(
        std::optional<unsigned> constraint, std::shared_ptr<State> s);
    static Result found_trace(std::optional<unsigned> constraint, State&& s);
    static Result found_invariant(
        std::optional<unsigned> constraint, int level);
    static Result empty_true();
    static Result empty_false();

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
    void finalize(const pebbling::Model& model);
    // iterators over the Trace. empty if there is an Invariant
    const_iterator begin();
    const_iterator end();

    class const_iterator
    {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = State;
      using const_pointer     = std::shared_ptr<const State>;
      using const_reference   = const State&;

     private:
      const_pointer m_ptr;

     public:
      const_iterator(const_pointer ptr);
      const_reference operator*() const;
      const_pointer operator->();
      const_iterator& operator++();
      const_iterator operator++(int);
      friend bool operator==(const const_iterator& a, const const_iterator& b);
      friend bool operator!=(const const_iterator& a, const const_iterator& b);
    };

   private:
    bool finalized  = false;
    std::string str = "";

    Result(std::optional<unsigned> constr, std::shared_ptr<State> s);
    Result(std::optional<unsigned> constr, int l);
  };

  // collection of >= 1 pdr runs on the same model with varying constraints
  // if decreasing: trace, trace, ..., invariant
  // if increasing: invariant, invariant, ..., trace
  class Results
  {
    using ResultRow = std::array<std::string, 5>;
    friend class ExperimentResults;

   protected:
    const pebbling::Model& model;
    const ResultRow header = { "constraint", "pebbles used", "invariant index",
      "trace length", "time" };
    std::vector<ResultRow> rows;

    std::vector<Result> original;
    std::vector<std::string> traces;

   public:
    Results(const pebbling::Model& m);
    TextTable new_table() const;
    void reset();
    virtual void show(std::ostream& out) const;
    Results& add(Result& r);
    friend Results& operator<<(Results& rs, Result& r);
    std::vector<double> extract_times() const;
  };

  // results that accumulates results for experiments
  class ExperimentResults : public Results
  {
   private:
    double total_time   = 0.0;
    int invariant_level = INT_MAX;  // earliest invariant found
    unsigned length     = UINT_MAX; // shortest trace found
    unsigned marked     = 0;
    const Tactic tactic;

    void acc_update(const Result& r);

   public:
    ExperimentResults(const pebbling::Model& m, Tactic t);
    ExperimentResults(const Results& r, Tactic t);
    void show(std::ostream& out) const override;
    void show_raw(std::ostream& out) const;
    ExperimentResults& add(Result& r);
    friend ExperimentResults& operator<<(ExperimentResults& rs, Result& r);
  };
} // namespace pdr
#endif // PDR_RES
