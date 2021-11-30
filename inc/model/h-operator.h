#ifndef H_OP
#define H_OP

#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include <mockturtle/io/write_dot.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>

#include "dag.h"
#include "parse_dot.h"

using namespace mockturtle;

namespace mockturtle
{
  template <class Ntk> class my_dot_drawer : public default_dot_drawer<Ntk>
  {
   public:
    virtual std::string node_label(Ntk const& ntk, node<Ntk> const& n) const
    {
      if (ntk.is_pi(n))
        return "in_" + std::to_string(ntk.node_to_index(n));
      if (ntk.is_constant(n))
        return "const_" + std::to_string(ntk.node_to_index(n));
      return std::to_string(ntk.node_to_index(n));
    }
  };
} // namespace mockturtle

// H-operator as described by https://arxiv.org/pdf/1904.02121.pdf
inline xmg_network build_hoperator(uint64_t bitwidth, uint64_t modulus)
{
  xmg_network xmg;
  std::vector<xmg_network::signal> a(bitwidth), b(bitwidth), c(bitwidth),
      d(bitwidth);
  std::vector<xmg_network::signal> x(bitwidth), y(bitwidth), z(bitwidth),
      t(bitwidth);
  std::vector<bool> m(bitwidth);
  bool_vector_from_dec(m, modulus);

  std::generate(a.begin(), a.end(), [&xmg]() { return xmg.create_pi(); });
  std::generate(b.begin(), b.end(), [&xmg]() { return xmg.create_pi(); });
  std::generate(c.begin(), c.end(), [&xmg]() { return xmg.create_pi(); });
  std::generate(d.begin(), d.end(), [&xmg]() { return xmg.create_pi(); });

  std::vector<xmg_network::signal> t1 =
      modular_adder(xmg, a, b, m); // t1 = a + b
  std::vector<xmg_network::signal> t2 =
      modular_adder(xmg, c, d, m); // t2 = c + d
  std::vector<xmg_network::signal> t3 =
      modular_subtractor(xmg, a, b, m); // t3 = a - b
  std::vector<xmg_network::signal> t4 =
      modular_subtractor(xmg, c, d, m); // t4 = c - d

  x = modular_adder(xmg, t1, t2, m);      // x = t1 + t2
  y = modular_subtractor(xmg, t1, t2, m); // y = t1 - t2
  z = modular_adder(xmg, t3, t4, m);      // z = t3 + t4
  t = modular_subtractor(xmg, t3, t4, m); // t = t3 - t4

  std::for_each(x.begin(), x.end(),
                [&xmg](auto f) { return xmg.create_po(f); });
  std::for_each(y.begin(), y.end(),
                [&xmg](auto f) { return xmg.create_po(f); });
  std::for_each(z.begin(), z.end(),
                [&xmg](auto f) { return xmg.create_po(f); });
  std::for_each(t.begin(), t.end(),
                [&xmg](auto f) { return xmg.create_po(f); });

  return cleanup_dangling(xmg);
}

namespace dag
{
  inline Graph hoperator(uint64_t bitwidth, uint64_t modulus)
  {
    std::stringstream dot_string;
    my_dot_drawer<xmg_network> drawer;
    write_dot(build_hoperator(bitwidth, modulus), dot_string, drawer);

    return parse::parse_dot(dot_string.str(), true);
  }
} // namespace dag

#endif // H_OP
