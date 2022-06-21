#include "peterson.h"
#include <fmt/color.h>

namespace peterson
{
  using z3::expr;

  Model::Model(z3::config& settings, unsigned n_processes)
      : ctx(settings), N(n_processes), pc(ctx), l(ctx), last(ctx), initial(ctx),
        transition(ctx)
  {
    using fmt::format;

    size_t n_bits = std::ceil(std::log2(N));

    for (unsigned i = 0; i < N; i++)
    {
      std::string pc_i = format("pc_{}", i);
      pc.push_back( ctx.bv_const(pc_i.c_str(), 2) );

      std::string l_i = format("l_{}", i);
      l.push_back( ctx.int_const(l_i.c_str()) );
      // pc.push_back( ctx.bv_const(l_i.c_str(), n_bits) );

      std::string last_i = format("last_{}", i);
      last.push_back( ctx.int_const(last_i.c_str()) );
      // pc.push_back( ctx.bv_const(last_i.c_str(), n_bits) );

      initial.push_back(pc[i] == 0);
      initial.push_back(l[i] < 0);
      initial.push_back(last[i] < 0);

      transition.push_back();
    }
  }

  expr Model::T_init(unsigned i)
  {
    
  }

  expr Model::T_boundcheck(unsigned i)
  {
    
  }
} // namespace peterson
