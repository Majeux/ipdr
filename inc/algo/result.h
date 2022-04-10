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
    class iterator;
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
    double total_time = 0.0;
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

    void clean_trace();
    ResultRow listing() const;
    std::string_view strategy_string(const PebblingModel& model);
    // iterators over the Trace. empty if there is an Invariant
    iterator begin();
    iterator end();

    class iterator
    {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = State;
      using pointer           = std::shared_ptr<State>;
      using reference         = State&;

     private:
      pointer m_ptr;

     public:
      iterator(pointer ptr);
      reference operator*() const;
      pointer operator->();
      iterator& operator++();
      iterator operator++(int);
      friend bool operator==(const iterator& a, const iterator& b);
      friend bool operator!=(const iterator& a, const iterator& b);
    };

   private:
    std::string str = "";

    Result(std::optional<unsigned> constr, std::shared_ptr<State> s);
    Result(std::optional<unsigned> constr, int l);
  };

  struct Results
  {
    const PebblingModel& model;
    TextTable table;
    std::vector<std::string> traces;

    Results(const PebblingModel& m);
    void show(std::ostream& out) const;
    friend Results& operator<<(Results& rs, Result& r);
  };
} // namespace pdr
#endif // PDR_RES
