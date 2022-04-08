#ifndef PDR_RES
#define PDR_RES

#include "TextTable.h"
#include "obligation.h"
#include "output.h"
#include "pdr-model.h"
#include <fmt/format.h>
#include <memory>
#include <sstream>
#include <vector>
namespace pdr
{
  // converts to bool: true if trace == null. if the property holds
  // iterating walks through the linked list starting with trace
  struct Result
  {
    unsigned constraint;
    double total_time;
    bool cleaned = false; // result contained a trace, but it was deleted

    // Trace struct
    std::shared_ptr<State> trace;
    std::string trace_string;
    unsigned trace_length;
    unsigned marked;
    
    // Inariant struct
    int invariant_level;

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
      iterator(pointer ptr) : m_ptr(ptr) {}
      reference operator*() const { return *m_ptr; }
      pointer operator->() { return m_ptr; }

      iterator& operator++()
      {
        m_ptr = m_ptr->prev;
        return *this;
      }

      iterator operator++(int)
      {
        iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      friend bool operator==(const iterator& a, const iterator& b)
      {
        return a.m_ptr == b.m_ptr;
      };
      friend bool operator!=(const iterator& a, const iterator& b)
      {
        return a.m_ptr != b.m_ptr;
      };
    };

    operator bool() const { return trace.get() == nullptr || cleaned; }

    Result()
        : constraint(0), trace(nullptr), trace_string(""), trace_length(0),
          marked(0), invariant_level(-1), total_time(0.0)
    {
    }

    void clean_trace()
    {
      assert(trace);
      trace.reset();
      cleaned = true;
    }

    static Result found_trace(std::shared_ptr<State> s) { return Result(s); }
    static Result found_trace(State&& s)
    {
      return Result(std::make_shared<State>(s));
    }
    static Result found_invariant(int l) { return Result(l); }
    static Result empty_true() { return Result(); }

    std::vector<std::string> listing() const
    {
      return { std::to_string(marked), std::to_string(invariant_level),
        std::to_string(trace_length), std::to_string(total_time) };
    }

    iterator begin() { return iterator(trace); }
    iterator end() { return iterator(nullptr); }

   private:
    Result(std::shared_ptr<State> s)
        : trace(s), trace_string(""), trace_length(0), marked(0),
          invariant_level(-1), total_time(0.0)
    {
    }

    Result(int l)
        : trace(nullptr), trace_string(""), trace_length(0), marked(0),
          invariant_level(l), total_time(0.0)
    {
    }
  };

  struct Results
  {
    TextTable table;
    std::vector<std::string> traces;
    Results()
    {
      std::vector<std::string> header = { "pebbles", "invariant index",
        "strategy length", "Total time" };
      for (unsigned i = 0; i < header.size(); i++)
        table.setAlignment(i, TextTable::Alignment::RIGHT);

      table.addRow(header);
    }

    friend Results& operator<<(Results& rs, const Result& r)
    {
      rs.table.addRow(r.listing());
      rs.traces.push_back(r.trace_string);
      return rs;
    }

    void show(std::ostream& out) const
    {
      out << table << std::endl << std::endl;

      for (const std::string& trace : traces)
        out << trace << std::endl;
    }
  };
} // namespace pdr
#endif // PDR_RES
