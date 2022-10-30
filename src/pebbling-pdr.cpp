#include "bounded.h"
#include "cli-parse.h"
#include "dag.h"
#include "h-operator.h"
#include "io.h"
#include "_logging.h"
#include "logger.h"
#include "mockturtle/networks/klut.hpp"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-experiments.h"
#include "pebbling-model.h"
#include "peterson-experiments.h"
#include "peterson.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cxxopts.hpp>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <ghc/filesystem.hpp>
#include <iostream>
#include <lorina/bench.hpp>
#include <memory>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <z3++.h>

using namespace my::cli;
using namespace my::io;

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

dag::Graph build_dag(const ArgumentList& args)
{
  dag::Graph G;
  // read input model
  switch (args.model)
  {
    case ModelType::hoperator:
      G = dag::hoperator(args.hop.bits, args.hop.mod);
      break;

    case ModelType::tfc:
    {
      parse::TFCParser parser;
      fs::path model_file = args.bench_folder / (args.model_name + ".tfc");
      G = parser.parse_file(model_file.string(), args.model_name);
    }
    break;

    case ModelType::bench:
    {
      fs::path model_file = args.bench_folder / (args.model_name + ".bench");
      mockturtle::klut_network klut;
      auto const result = lorina::read_bench(
          model_file.string(), mockturtle::bench_reader(klut));
      if (result != lorina::return_code::success)
        throw std::invalid_argument(
            model_file.string() + " is not a valid .bench file");

      G = dag::from_dot(klut, args.model_name); // TODO continue
    }
    break;

    default: break;
  }

  return G;
}

std::ostream& operator<<(std::ostream& o, std::exception const& e)
{
  o << fmt::format(
           "terminated after throwing an \'std::exception\', typeid: {}",
           typeid(e).name())
    << std::endl
    << fmt::format("  what():  {}", e.what()) << std::endl;
  return o;
}

////////////////////////////////////////////////////////////////////////////////

dag::Graph setup_graph(const ArgumentList& clargs)
{
  // create folders and files for I/O
  fs::path model_dir        = create_model_dir(clargs);
  std::ofstream graph_descr = trunc_file(model_dir, "graph", "txt");

  dag::Graph G = build_dag(clargs);
  {
    G.show_image(model_dir / "dag");
    std::cout << G.summary() << std::endl;
    graph_descr << G.summary() << std::endl << G;
  }

  return G;
}

void experiment(ArgumentList& clargs)
{
  using std::ofstream;
  using std::string;

  std::cerr << "experiment()" << std::endl;

  dag::Graph G = build_dag(clargs);

  pdr::pebbling::PebblingModel model(clargs, G);

  const fs::path model_dir = create_model_dir(clargs);
  const string filename    = file_name(clargs);
  const fs::path run_dir   = setup(model_dir / run_folder_name(clargs));

  string sub = "analysis";
  fs::create_directory(run_dir / sub);
  ofstream stat_file = trunc_file(run_dir / sub, filename, "stats");
  ofstream strategy  = trunc_file(run_dir / sub, filename, "strategy");
  fs::path log_file  = run_dir / sub / fmt::format("{}.log", filename);

  pdr::Statistics stats =
      pdr::Statistics::PebblingStatistics(std::move(stat_file), G);
  pdr::Logger logger =
      pdr::Logger(log_file.string(), clargs.verbosity, std::move(stats));

  pdr::pebbling::experiments::pebbling_run(model, logger, clargs);
  std::cout << "experiment done" << std::endl;
}

void peter_experiment(ArgumentList& clargs)
{
  using std::ofstream;
  using std::string;

  std::cerr << "peter_experiment()" << std::endl;

  unsigned p = 3, N = 3;
  pdr::peterson::PetersonModel model(p, N);
  clargs.model_name = fmt::format("peter-p{}-N{}-relax", p, N);

  const FolderStructure folders = FolderStructure::make_from(clargs);
  {
    auto model_logger = spdlog::basic_logger_st(
        "model_dump", folders.file_in_model(clargs.model_name, "model"), true);
    model_logger->set_level(spdlog::level::trace);
    spdlog::set_pattern("");

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

  ofstream stat_file = trunc_file(folders.analysis, folders.file_base, "stats");
  ofstream trace_file =
      trunc_file(folders.analysis, folders.file_base, "trace");
  fs::path log_file = folders.analysis / folders.file("log");

  folders.show(std::cerr);

  pdr::Statistics stats =
      pdr::Statistics::PeterStatistics(std::move(stat_file), p, N);
  pdr::Logger logger =
      pdr::Logger(log_file.string(), clargs.verbosity, std::move(stats));

  // return;
  pdr::peterson::experiments::peterson_run(model, logger, clargs);
  std::cout << "experiment done" << std::endl;
}

void bounded_experiment(const dag::Graph& G, ArgumentList& clargs)
{
  bounded::BoundedPebbling obj(G, clargs);
  obj.find_for(G.nodes.size());
}

#warning dont cares (?) in trace for non-tseytin. dont always make sense? mainly in high constraints
int main(int argc, char* argv[])
{
  using std::ofstream;

  ArgumentList clargs = parse_cl(argc, argv);

  if (clargs.peter)
  {
    peter_experiment(clargs);
    return 0;
  }

  if (clargs.exp_sample)
  {
    experiment(clargs);
    return 0;
  }

  const fs::path model_dir = create_model_dir(clargs);
  const dag::Graph G       = setup_graph(clargs);

  if (clargs.bounded)
  {
    bounded_experiment(G, clargs);
    return 0;
  }

  pdr::pebbling::PebblingModel model(clargs, G);
  pdr::Context context = clargs.seed
                           ? pdr::Context(model, clargs.delta, *clargs.seed)
                           : pdr::Context(model, clargs.delta, clargs.rand);

  ofstream model_descr = trunc_file(model_dir, "model", "txt");
  model.show(model_descr);

  if (clargs.onlyshow)
    return 0;

  const std::string filename = file_name(clargs);
  fs::path run_dir           = setup(model_dir / run_folder_name(clargs));

  ofstream stat_file   = trunc_file(run_dir, filename, "stats");
  ofstream strat_file  = trunc_file(run_dir, filename, "strategy");
  ofstream solver_dump = trunc_file(run_dir, "solver_dump", "solver");

  // initialize logger and other bookkeeping
  fs::path log_file = run_dir / fmt::format("{}.log", filename);

  pdr::Statistics stats =
      pdr::Statistics::PebblingStatistics(std::move(stat_file), G);

  pdr::Logger logger = clargs.out ? pdr::Logger(log_file.string(), *clargs.out,
                                        clargs.verbosity, std::move(stats))
                                  : pdr::Logger(log_file.string(),
                                        clargs.verbosity, std::move(stats));

  show_header(clargs);

  pdr::PDR algorithm(context, model, logger);

  if (clargs.tactic == pdr::Tactic::basic)
  {
    pdr::IpdrResult rs(model);
    model.constrain(clargs.starting_value);
    pdr::PdrResult r = algorithm.run();
    rs.add(r).show(strat_file);
    rs.show_traces(strat_file);
    algorithm.show_solver(solver_dump);

    return 0;
  }
  else
  {
    pdr::pebbling::IPDR optimize(context, model, clargs, logger);
    pdr::pebbling::PebblingResult result =
        optimize.run(pdr::Tactic::decrement, false);

    result.show(strat_file);
    result.show_raw(strat_file);
    result.show_traces(strat_file);
    optimize.dump_solver(solver_dump);
  }

  std::cout << "run done" << std::endl;
  return 0;
}
