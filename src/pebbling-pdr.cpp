#define _TRACE

#include "bounded.h"
#include "cli-parse.h"
#include "dag.h"
#include "experiments.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "mockturtle/networks/klut.hpp"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-experiments.h"
#include "pebbling-model.h"
#include "peterson-experiments.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "types-ext.h"
#include "vpdr.h"
#include "z3-pebbling-experiments.h"
#include "z3-pebbling-model.h"
#include "z3pdr.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cxxopts.hpp>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <string>
#include <tabulate/table.hpp>
#include <tuple>
#include <variant>
#include <vector>
#include <z3++.h>

using namespace my::cli;
using namespace my::io;

// aliases
using ModelVariant = std::variant<pdr::test::Z3PebblingModel,
    pdr::pebbling::PebblingModel, pdr::peterson::PetersonModel>;

// algorithm handling
ModelVariant construct_model(
    ArgumentList& args, pdr::Context& context, pdr::Logger& log);
void handle_pdr(ArgumentList& args, pdr::Context&& context, pdr::Logger& log);
void handle_ipdr(ArgumentList& args, pdr::Context&& context, pdr::Logger& log);
void handle_experiment(ArgumentList& args, pdr::Logger& log);
//
// aux
void show_files(std::ostream& os, std::map<std::string, fs::path> paths);
std::ostream& operator<<(std::ostream& o, std::exception const& e);

#warning dont cares (?) in trace for non-tseytin. dont always make sense? mainly in high constraints
int main(int argc, char* argv[])
{
  ArgumentList args(argc, argv);

  if (args.onlyshow)
    return 0;

  std::cout << args.folders.trace_file.is_open() << std::endl;

  args.show_header(std::cerr);
  args.folders.show(std::cerr);

  pdr::Logger logger = pdr::Logger(args.folders.file_in_analysis("log"),
      args.out, args.verbosity,
      pdr::Statistics(args.folders.file_in_analysis("stats")));

  if (args.experiment)
  {
    handle_experiment(args, logger);
    return 0;
  }

  z3::context ctx;
  pdr::Context context = std::holds_alternative<bool>(args.r_seed)
                           ? pdr::Context(ctx, std::get<bool>(args.r_seed))
                           : pdr::Context(ctx, std::get<unsigned>(args.r_seed));

  if (std::holds_alternative<algo::t_PDR>(args.algorithm))
    handle_pdr(args, std::move(context), logger);
  else if (std::holds_alternative<algo::t_IPDR>(args.algorithm))
    handle_ipdr(args, std::move(context), logger);

  // if (auto peb_descr = get_cref<model_t::Pebbling>(args.model))
  //   handle_pebbling(peb_descr->get(), args, context, logger);
  // else
  // {
  //   using namespace pdr::peterson;
  //   auto const& peter_descr = std::get<model_t::Peterson>(args.model);
  //   handle_peterson(peter_descr, args, context, logger);
  // }

  std::cout << "goodbye :)" << std::endl;
  return 0;
}

void show_files(std::ostream& os, std::map<std::string, fs::path> paths)
{
  // show used paths
  TextTable output_files;
  for (auto kv : paths)
  {
    auto row = { kv.first, kv.second.string() };
    output_files.addRow(row);
  }
  os << output_files << std::endl;
}

//
// end OUTPUT

std::ostream& operator<<(std::ostream& o, std::exception const& e)
{
  o << fmt::format(
           "terminated after throwing an \'std::exception\', typeid: {}",
           typeid(e).name())
    << std::endl
    << fmt::format("  what():  {}", e.what()) << std::endl;
  return o;
}

using ModelVariant = std::variant<pdr::test::Z3PebblingModel,
    pdr::pebbling::PebblingModel, pdr::peterson::PetersonModel>;

ModelVariant construct_model(
    ArgumentList& args, pdr::Context& context, pdr::Logger& log)
{
  using my::variant::get_cref;

  if (auto pebbling = get_cref<model_t::Pebbling>(args.model))
  {
    dag::Graph G = model_t::make_graph(pebbling->get().src);
    G.show(args.folders.model_dir / "dag", true);
    log.stats.is_pebbling(G);

    if (args.z3pdr)
    {
      return pdr::test::Z3PebblingModel(args, context.z3_ctx, G)
          .constrained(pebbling->get().max_pebbles);
    }
    else
    {
      return pdr::pebbling::PebblingModel(args, context.z3_ctx, G)
          .constrained(pebbling->get().max_pebbles);
    }
  }

  auto peterson = get_cref<model_t::Peterson>(args.model);
  assert(peterson);

  unsigned start = peterson->get().start;
  unsigned max   = peterson->get().max;
  pdr::peterson::PetersonModel peter(context.z3_ctx, start, max);
  log.stats.is_peter(start, max);
  peter.show(args.folders.model_file);

  return peter;
}

// (https://en.cppreference.com/w/cpp/utility/variant/visit)
// helper type for std::visitor
// allows packaging of lambdas without creating a functor struct:
// visitor { lambda1, lambda2, ... }
template <class... Ts> struct visitor : Ts...
{
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts> visitor(Ts...) -> visitor<Ts...>;

void handle_pdr(ArgumentList& args, pdr::Context&& context, pdr::Logger& log)
{
  using namespace pdr;
  using std::endl;

  using PDRVariant = std::variant<PDR, test::z3PDR>;

  ModelVariant model   = construct_model(args, context, log);
  PDRVariant algorithm = std::visit(
      visitor{
          [&](test::Z3PebblingModel& m) -> PDRVariant
          { return test::z3PDR(context, log, m); },
          [&](auto& m) -> PDRVariant { return PDR(args, context, log, m); },
      },
      model);

  pdr::PdrResult res = std::visit([](vPDR& a) { return a.run(); }, algorithm);
  std::cout << "result" << std::endl;
  tabulate::Table T = res.get_table();

  std::cout << T << endl << endl;
  args.folders.trace_file << T << endl << endl;

  std::string trace =
      std::visit(visitor{ [&](test::Z3PebblingModel const& m) {
                           return pdr::result::trace_table(
                               res, m.vars.names(), m.vars.names_p());
                         },
                     [&](IModel const& m) {
                       return pdr::result::trace_table(
                           res, m.vars.names(), m.vars.names_p());
                     } },
          model);
  std::cout << trace;
  args.folders.trace_file << trace;

  std::visit(
      [&](vPDR& a) { a.show_solver(args.folders.solver_dump); }, algorithm);
}

void handle_ipdr(ArgumentList& args, pdr::Context&& context, pdr::Logger& log)
{
  using namespace pdr;
  using std::endl;
  using IPDRVariant =
      std::variant<test::z3PebblingIPDR, pebbling::IPDR, peterson::IPDR>;
  using ResultVariant =
      std::variant<pebbling::IpdrPebblingResult, peterson::IpdrPetersonResult>;

  using my::variant::get_cref;
  auto const& ipdr = get_cref<algo::t_IPDR>(args.algorithm)->get();

  ModelVariant model    = construct_model(args, context, log);
  IPDRVariant algorithm = std::visit(
      visitor{
          [&](test::Z3PebblingModel& m) -> IPDRVariant
          { return test::z3PebblingIPDR(args, context, log, m); },
          [&](pebbling::PebblingModel& m) -> IPDRVariant
          { return pebbling::IPDR(args, context, log, m); },
          [&](peterson::PetersonModel& m) -> IPDRVariant
          { return peterson::IPDR(args, context, log, m); },
      },
      model);

  ResultVariant result = std::visit(
      visitor{
          [&](test::z3PebblingIPDR& a) -> ResultVariant
          { return a.control_run(ipdr.type); },
          [&](pebbling::IPDR& a) -> ResultVariant
          { return a.run(ipdr.type, false); },
          [&](peterson::IPDR& a) -> ResultVariant
          { return a.run(ipdr.type, {}, false); },
      },
      algorithm);

  std::visit(
      [&](IpdrResult const& r)
      {
        args.folders.trace_file << r.end_result() << endl
                                << r.summary_table() << endl
                                << std::string(20, '=') << endl
                                << r.all_traces() << endl;
      },
      result);

  std::visit(
      visitor{
          [&](test::z3PebblingIPDR const& a)
          { a.internal_alg().show_solver(args.folders.solver_dump); },
          [&](vIPDR const& a)
          { a.internal_alg().show_solver(args.folders.solver_dump); },
      },
      algorithm);
}

void handle_experiment(ArgumentList& args, pdr::Logger& log)
{
  using std::make_unique;
  using std::unique_ptr;
  using namespace pdr;
  using pdr::pebbling::PebblingModel;
  using pdr::pebbling::experiments::PebblingExperiment;
  using pdr::peterson::PetersonModel;
  using pdr::peterson::experiments::PetersonExperiment;
  using pdr::test::Z3PebblingModel;
  using pdr::test::experiments::Z3PebblingExperiment;

  using GeneralExperimentPtr = unique_ptr<experiments::Experiment>;

  using my::variant::get_cref;

  GeneralExperimentPtr experiment = [&]() -> GeneralExperimentPtr
  {
    if (auto pebbling = get_cref<model_t::Pebbling>(args.model))
    {
      if (args.z3pdr)
      {
        return make_unique<Z3PebblingExperiment>(args, log);
      }
      return make_unique<PebblingExperiment>(args, log);
    }
    return make_unique<PetersonExperiment>(args, log);
  }();

  experiment->run();
}

// void handle_pebbling(model_t::Pebbling const& descr, ArgumentList& args,
//     pdr::Context context, pdr::Logger& log)
// {
//   using namespace pdr::pebbling;
//   using my::variant::get_cref;
//   using my::variant::get_ref;
//   using pdr::test::Z3PebblingModel;
//   using std::endl;
//   using std::make_unique;
//   using std::unique_ptr;

//   assert(!args.z3pdr);

//   dag::Graph G = model_t::make_graph(descr.src);
//   G.show(args.folders.model_dir / "dag", true);
//   log.stats.is_pebbling(G);

//   auto determine_model = [&]() -> std::variant<Z3PebblingModel,
//   PebblingModel>
//   {
//     if (args.z3pdr)
//       return Z3PebblingModel(args, context.z3_ctx, G);
//     else
//       return PebblingModel(args, context.z3_ctx, G);
//   };
//   std::variant<Z3PebblingModel, PebblingModel> pebbling = determine_model();

//   auto show = [&](auto const& var) { var.show(args.folders.model_file); };
//   std::visit(show, pebbling);

//   if (std::holds_alternative<algo::t_PDR>(args.algorithm))
//   {
//     auto constrain = [&](auto& var) { var.constrain(descr.max_pebbles); };
//     std::visit(constrain, pebbling);

//     unique_ptr<pdr::vPDR> pdr_algo;
//     if (auto peb = get_ref<PebblingModel>(pebbling))
//       pdr_algo = make_unique<pdr::PDR>(args, context, log, peb->get());
//     else if (auto z3peb = get_ref<Z3PebblingModel>(pebbling))
//       pdr_algo = make_unique<pdr::test::z3PDR>(context, log, z3peb->get());
//     else
//       assert(false);

//     pdr::PdrResult r = pdr_algo->run();
//     {
//       std::cout << "result" << std::endl;
//       tabulate::Table T;
//       T.add_row(
//           { pdr::PdrResult::fields.cbegin(), pdr::PdrResult::fields.cend()
//           });
//       auto const row2 = r.listing();
//       T.add_row({ row2.cbegin(), row2.cend() });

//       std::cout << T << endl << endl;
//       args.folders.trace_file << T << endl << endl;

//       if (auto peb = get_ref<PebblingModel>(pebbling))
//       {
//         std::string trace = pdr::result::trace_table(
//             r, peb->get().vars, peb->get().get_initial());
//         std::cout << trace;
//         args.folders.trace_file << trace;
//       }

//       pdr_algo->show_solver(args.folders.solver_dump);
//     }
//   }
//   else if (auto ipdr = get_cref<algo::t_IPDR>(args.algorithm))
//   {
//     if (auto peb = get_ref<PebblingModel>(pebbling))
//     {
//       if (args.experiment)
//       {
//         pdr::pebbling::experiments::PebblingExperiment exp(
//             args, peb->get(), log);
//         exp.run();
//       }
//       else
//       {
//         pdr::pebbling::IPDR ipdr_algo(context, peb->get(), args, log);
//         pdr::pebbling::PebblingResult result =
//             ipdr_algo.run(ipdr->get().type, false);

//         args.folders.trace_file << result.end_result() << endl
//                                 << result.summary_table() << endl
//                                 << std::string(20, '=') << endl
//                                 << result.all_traces() << endl;

//         ipdr_algo.internal_alg().show_solver(args.folders.solver_dump);
//       }
//     }
//     else
//     {
//       assert(false && "todo");
//     }
//   }
//   else
//   {
//     assert(std::holds_alternative<algo::t_Bounded>(args.algorithm));
//     bounded::BoundedPebbling obj(G, args);
//     obj.find_for(G.nodes.size());
//   }
// }

// void handle_peterson(model_t::Peterson const& descr, ArgumentList& args,
//     pdr::Context context, pdr::Logger& log)
// {
//   using namespace pdr::peterson;
//   using my::variant::get_cref;
//   using std::endl;
//   using std::make_unique;
//   using std::unique_ptr;

//   PetersonModel peter(context.z3_ctx, descr.start, descr.max);
//   log.stats.is_peter(descr.start, descr.max);
//   peter.show(args.folders.model_file);

//   if (args.z3pdr)
//   {
//     std::cerr << "z3 pdr peterson" << std::endl;
//     std::cerr << "not implemented" << std::endl;
//     std::cerr << "ignoring other arguments" << std::endl;
//     return;
//   }

//   if (std::holds_alternative<algo::t_PDR>(args.algorithm))
//   {
//     unique_ptr<pdr::vPDR> pdr_algo;
//     if (args.z3pdr)
//     {
//       std::cout << "z3 peterson not implemented" << std::endl;
//       // pdr_algo = make_unique<pdr::test::z3PDR>(context, log);
//     }
//     else
//       pdr_algo = make_unique<pdr::PDR>(context, log, peter);

//     pdr::PdrResult r = pdr_algo->run();
//     {
//       tabulate::Table T;
//       {
//         T.add_row(
//             { pdr::PdrResult::fields.cbegin(), pdr::PdrResult::fields.cend()
//             });
//         auto const row2 = r.listing();
//         T.add_row({ row2.cbegin(), row2.cend() });
//       }
//       args.folders.trace_file << T << endl << endl;
//       args.folders.trace_file
//           << pdr::result::trace_table(r, peter.vars, peter.get_initial());
//       pdr_algo->show_solver(args.folders.solver_dump);
//     }
//   }
//   else if (auto ipdr = get_cref<algo::t_IPDR>(args.algorithm))
//   {
//     if (args.experiment)
//     {
//       experiments::PetersonExperiment exp(args, peter, log);
//       exp.run();
//     }
//     else
//     {
//       using std::endl;
//       std::vector<std::unique_ptr<pdr::IpdrResult>> test;

//       IPDR ipdr_algo(context, peter, args, log);
//       PetersonResult result = ipdr_algo.run(ipdr->get().type, false);

//       args.folders.trace_file << result.end_result() << endl
//                               << result.summary_table() << endl
//                               << std::string('=', 20) << endl
//                               << result.all_traces() << endl;

//       ipdr_algo.internal_alg().show_solver(args.folders.solver_dump);
//     }
//   }
//   assert(false && "TODO: peterson control flow");
// }

////////////////////////////////////////////////////////////////////////////////

void show_peter_model(
    fs::path const& out, pdr::peterson::PetersonModel const& model)
{
  auto model_logger = spdlog::basic_logger_st("model_dump", out, true);
  model_logger->set_level(spdlog::level::trace);
  spdlog::set_pattern("%v");

  tabulate::Table t;
  t.add_row({ "I", model.get_initial().to_string() });
  t.add_row(
      { "P", model.property().to_string(), model.property.p().to_string() });
  t.add_row({ "!P", model.n_property().to_string(),
      model.n_property.p().to_string() });
  // t.add_row({ "T", model.get_constraint().to_string() });
  SPDLOG_LOGGER_TRACE(model_logger, t.str());
  model_logger->flush();
}
