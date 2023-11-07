#include "cli-parse.h"
#include "dag.h"
#include "experiments.h"
#include "expr.h"
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
using ModelVariant =
    std::variant<pdr::pebbling::PebblingModel, pdr::peterson::PetersonModel>;

// algorithm handling
ModelVariant construct_model(
    ArgumentList& args, pdr::Context& context, pdr::Logger& log);
void handle_pdr(ArgumentList& args, pdr::Context context, pdr::Logger& log);
void handle_ipdr(ArgumentList& args, pdr::Context context, pdr::Logger& log);
void handle_experiment(ArgumentList& args, pdr::Logger& log);
//
// aux
void show_files(std::ostream& os, std::map<std::string, fs::path> paths);
std::ostream& operator<<(std::ostream& o, std::exception const& e);

int main(int argc, char* argv[])
{
  ArgumentList args(argc, argv);

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
  pdr::Context context(ctx, args);

  if (std::holds_alternative<algo::t_PDR>(args.algorithm))
    handle_pdr(args, std::move(context), logger);
  else if (std::holds_alternative<algo::t_IPDR>(args.algorithm))
    handle_ipdr(args, std::move(context), logger);

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

ModelVariant construct_model(
    ArgumentList& args, pdr::Context& context, pdr::Logger& log)
{
  using my::variant::get_cref;

  if (auto pebbling = get_cref<model_t::Pebbling>(args.model))
  {
    dag::Graph G = model_t::make_graph(pebbling->get().src);
    G.show(args.folders.model_dir / "dag", true, args.onlyshow);
    log.stats.is_pebbling(G);

    return pdr::pebbling::PebblingModel(args, context.z3_ctx, G)
        .constrained(pebbling->get().max_pebbles);
  }

  auto peterson = get_cref<model_t::Peterson>(args.model);
  assert(peterson);

  unsigned procs        = peterson->get().processes;
  unsigned switch_bound = peterson->get().switch_bound.value();

  auto peter = pdr::peterson::PetersonModel::constrained_switches(
      context.z3_ctx, procs, switch_bound);
  log.stats.is_peter(procs, switch_bound);
  peter.show(args.folders.model_file);
  // peter.test_room();

  return peter;
}

void handle_pdr(ArgumentList& args, pdr::Context context, pdr::Logger& log)
{
  using namespace pdr;
  using my::variant::visitor;
  using std::endl;

  using PDRVariant = std::variant<PDR, test::z3PDR>;

  ModelVariant model = construct_model(args, context, log);

  if (args.onlyshow)
    return;

  PDRVariant algorithm = std::visit(
      visitor{
          [&](auto& m) -> PDRVariant
          {
            if (args.z3pdr)
              return test::z3PDR(context, log, m);
            else
              return PDR(args, context, log, m);
          },
      },
      model);

  std::string model_name = model_t::get_name(args.model);
  log.graph.reset(model_name, "pdr");

  pdr::PdrResult res = std::visit([](vPDR& a) { return a.run(); }, algorithm);

  // write stat graph
  std::ofstream graph = args.folders.file_in_analysis("tex");
  graph << log.graph.get();

  std::cout << "result" << std::endl;
  tabulate::Table T = res.get_table();

  std::cout << T << endl << endl;
  args.folders.trace_file << T << endl << endl;

  std::string trace = std::visit(visitor{ [&](pebbling::PebblingModel const& m)
                                     {
                                       return pebbling::result::trace_table(res,
                                           m.vars.names(), m.vars.names_p(), m);
                                     },
                                     [&](peterson::PetersonModel const& m)
                                     {
                                       return peterson::result::trace_table(res,
                                           m.vars.names(), m.vars.names_p(), m);
                                     },
                                     [&](IModel const& m) {
                                       return pdr::result::trace_table(res,
                                           m.vars.names(), m.vars.names_p());
                                     } },
      model);
  std::cout << trace;
  args.folders.trace_file << trace;

  std::visit(
      [&](vPDR& a) { a.show_solver(args.folders.solver_dump); }, algorithm);
}

void handle_ipdr(ArgumentList& args, pdr::Context context, pdr::Logger& log)
{
  using namespace pdr;
  using namespace my::variant;
  using std::endl;
  using IPDRVariant = std::variant<pebbling::IPDR, peterson::IPDR>;
  using ResultVariant =
      std::variant<pebbling::IpdrPebblingResult, peterson::IpdrPetersonResult>;

  auto const& ipdr = get_cref<algo::t_IPDR>(args.algorithm)->get();

  ModelVariant model(construct_model(args, context, log));

  if (args.onlyshow)
    return;

  std::string model_name  = model_t::get_name(args.model);
  std::string tactic_name = pdr::tactic::to_string(ipdr.type);
  log.graph.reset(model_name, tactic_name);

  // create algorithm
  IPDRVariant algorithm(std::visit(
      visitor{
          [&](pebbling::PebblingModel& m) -> IPDRVariant
          { return pebbling::IPDR(args, context, log, m); },
          [&](peterson::PetersonModel& m) -> IPDRVariant
          { return peterson::IPDR(args, context, log, m); },
      },
      model));

  // run
  ResultVariant result(std::visit(
      visitor{
          [&](pebbling::IPDR& a) -> ResultVariant { return a.run(ipdr.type); },
          [&](peterson::IPDR& a) -> ResultVariant
          {
            unsigned max_switches = get_cref<model_t::Peterson>(args.model)
                                        ->get()
                                        .switch_bound.value();
            return a.run(ipdr.type, max_switches);
          },
      },
      algorithm));

  // write stat graph
  std::ofstream graph = args.folders.file_in_analysis("tex");
  graph << log.graph.get();

  // write result
  std::visit(
      [&](IpdrResult const& r)
      {
        args.folders.trace_file << r.end_result() << endl
                                << r.summary_table() << endl
                                << std::string(20, '=') << endl
                                << r.all_traces() << endl;
      },
      result);

  // write solver state
  std::visit(
      visitor{
          [&](vIPDR const& a)
          { a.internal_alg().show_solver(args.folders.solver_dump); },
      },
      algorithm);
}

void handle_experiment(ArgumentList& args, pdr::Logger& log)
{
  using namespace pdr;
  using my::variant::get_cref;
  using pdr::pebbling::experiments::PebblingExperiment;
  using pdr::peterson::experiments::PetersonExperiment;
  using std::make_unique;
  using std::unique_ptr;

  auto experiment = [&]() -> unique_ptr<experiments::Experiment>
  {
    if (auto pebbling = get_cref<model_t::Pebbling>(args.model))
    {
      return make_unique<PebblingExperiment>(args, log);
    }
    else
      return make_unique<PetersonExperiment>(args, log);
  }();

  experiment->run();
}

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
