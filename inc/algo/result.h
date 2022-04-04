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
  // converts to bool: true if trace != null
  // iterating walks through the linked list starting with trace
  struct Result
  {
    std::shared_ptr<State> trace;
    std::string trace_string;
    unsigned trace_length;
    unsigned pebbles_used;
    int invariant_index;
    double total_time;

    struct iterator
    {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = State;
      using pointer           = std::shared_ptr<State>;
      using reference         = State&;

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

     private:
      pointer m_ptr;
    };

    Result()
        : trace(nullptr), trace_string(""), trace_length(0), pebbles_used(0),
          invariant_index(-1), total_time(0.0)
    {
    }

    operator bool() const { return trace.get() != nullptr; }

    std::vector<std::string> listing() const
    {
      return { std::to_string(pebbles_used), std::to_string(invariant_index),
        std::to_string(trace_length), std::to_string(total_time) };
    }

    iterator begin() { return iterator(trace); }
    iterator end() { return iterator(nullptr); }
  };

  struct Results
  {
    std::vector<Result> vec;

    Results() : vec(1) {}

    Result& current() { return vec.back(); }
    void extend() { vec.emplace_back(); }

    void show(std::ostream& out) const
    {
      TextTable t;

      std::vector<std::string> header = { "pebbles", "invariant index",
        "strategy length", "Total time" };
      for (unsigned i = 0; i < header.size(); i++)
        t.setAlignment(i, TextTable::Alignment::RIGHT);

      t.addRow(header);
      for (const Result& res : vec)
        t.addRow(res.listing());

      out << t << std::endl << std::endl;

      for (const Result& res : vec)
        out << res.trace_string << std::endl;
    }
  };
} // namespace pdr
#endif // PDR_RES
