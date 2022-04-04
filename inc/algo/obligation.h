#ifndef PDR_OBL_H
#define PDR_OBL_H

#include "TextTable.h"
#include "z3-ext.h"
#include <fmt/format.h>
#include <numeric>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  struct State
  {
    z3::expr_vector cube;
    std::shared_ptr<State> prev; // store predecessor for trace

    State(const z3::expr_vector& e) : cube(e), prev(std::shared_ptr<State>()) {}
    State(const z3::expr_vector& e, std::shared_ptr<State> s) : cube(e), prev(s)
    {
    }
    // move constructors
    State(z3::expr_vector&& e)
        : cube(std::move(e)), prev(std::shared_ptr<State>())
    {
    }
    State(z3::expr_vector&& e, std::shared_ptr<State> s)
        : cube(std::move(e)), prev(s)
    {
    }

    std::vector<std::string> marking(
        std::vector<std::string> header, unsigned width) const
    {
      std::vector<std::string> rv(header.size(), "?");
      for (const z3::expr& e : cube)
      {
        std::string s = e.is_not() ? e.arg(0).to_string() : e.to_string();
        auto it       = std::lower_bound(header.begin(), header.end(), s);
        if (it != header.end() && *it == s) // it points to s
        {
          std::string fill_X      = fmt::format("{:X^{}}", "", width);
          rv[it - header.begin()] = e.is_not() ? "" : fill_X;
        }
      }

      return rv;
    }
    unsigned no_marked() const
    {
      return std::accumulate(cube.begin(), cube.end(), 0,
          [](int x, const z3::expr& e) { return x + (!e.is_not()); });
    }

    unsigned show(TextTable& table) const
    {
      std::vector<std::tuple<unsigned, std::string, unsigned>> steps;

      auto count_pebbled = [](const z3::expr_vector& vec)
      {
        unsigned count = 0;
        for (const z3::expr& e : vec)
          if (!e.is_not())
            count++;

        return count;
      };

      unsigned i = 1;
      steps.emplace_back(i, z3ext::join_expr_vec(cube), count_pebbled(cube));

      std::shared_ptr<State> current = prev;
      while (current)
      {
        i++;
        steps.emplace_back(i, z3ext::join_expr_vec(current->cube),
            count_pebbled(current->cube));
        current = current->prev;
      }
      unsigned i_padding = i / 10 + 1;

      std::string line_form = "{:>{}} |\t [ {} ] No. pebbled = {}";
      for (const auto& [num, vec, count] : steps)
      {
        std::vector<std::string> step_row = { std::to_string(num), vec,
          std::to_string(count) };
        table.addRow(step_row);
      }

      return i_padding;
    }
  };

  struct Obligation
  {
    unsigned level;
    std::shared_ptr<State> state;
    unsigned depth;

    Obligation(unsigned k, z3::expr_vector&& cube, unsigned d)
        : level(k), state(std::make_shared<State>(std::move(cube))), depth(d)
    {
    }

    Obligation(unsigned k, const std::shared_ptr<State>& s, unsigned d)
        : level(k), state(s), depth(d)
    {
    }

    // bool operator<(const Obligation& o) const { return this->level <
    // o.level; }
    bool operator<(const Obligation& o) const
    {
      if (this->level < o.level)
        return true;
      if (this->level > o.level)
        return false;
      if (this->depth < o.depth)
        return true;
      if (this->depth > o.depth)
        return false;

      return z3ext::expr_vector_less()(this->state->cube, o.state->cube);
    }
  };
} // namespace pdr
#endif // PDR_OBL
