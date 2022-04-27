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
#include "pdr-model.h"
#include "pdr.h"

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
  fs::path model_dir        = setup_model_path(clargs);
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
  using namespace pdr::experiments;
  using std::ofstream;
  using std::string;

  dag::Graph G = build_dag(clargs);

  static z3::config ctx_settings;
  {
    ctx_settings.set("unsat_core", true);
    ctx_settings.set("model", true);
  }
  pdr::pebbling::Model model(ctx_settings, clargs, G);

  const fs::path model_dir = setup_model_path(clargs);
  const string filename    = file_name(clargs);
  const fs::path run_dir   = setup_path(model_dir / folder_name(clargs));

  string sub        = "analysis";
  ofstream stats    = trunc_file(run_dir / sub, filename, "stats");
  ofstream strategy = trunc_file(run_dir / sub, filename, "strategy");

  fs::path log_file = run_dir / fmt::format("{}.log", filename);
  pdr::Logger logger =
      pdr::Logger(log_file.string(), G, clargs.verbosity, std::move(stats));

  model_run(model, logger, clargs);
  std::cout << "experiment done" << std::endl;
}

#warning dont cares (?) in trace for non-tseytin. dont always make sense? mainly in high constraints
int main(int argc, char* argv[])
{
  using std::ofstream;

  ArgumentList clargs = parse_cl(argc, argv);

  if (clargs.exp_sample)
  {
    experiment(clargs);
    return 0;
  }

  const fs::path model_dir = setup_model_path(clargs);
  const dag::Graph G       = setup_graph(clargs);

  z3::config ctx_settings;
  {
    ctx_settings.set("unsat_core", true);
    ctx_settings.set("model", true);
  }
  pdr::pebbling::Model model(ctx_settings, clargs, G);
  ofstream model_descr = trunc_file(model_dir, "model", "txt");
  model.show(model_descr);

  if (clargs.onlyshow)
    return 0;

  pdr::Context context = clargs.seed
                           ? pdr::Context(model, clargs.delta, *clargs.seed)
                           : pdr::Context(model, clargs.delta, clargs.rand);

  const std::string filename = file_name(clargs);
  fs::path run_dir           = setup_path(model_dir / folder_name(clargs));

  ofstream stats       = trunc_file(run_dir, filename, "stats");
  ofstream strategy    = trunc_file(run_dir, filename, "strategy");
  ofstream solver_dump = trunc_file(run_dir, "solver_dump", "solver");

  // initialize logger and other bookkeeping
  fs::path log_file = run_dir / fmt::format("{}.log", filename);

  pdr::Logger logger = clargs.out
                         ? pdr::Logger(log_file.string(), G, *clargs.out,
                               clargs.verbosity, std::move(stats))
                         : pdr::Logger(log_file.string(), G, clargs.verbosity,
                               std::move(stats));

  show_header(clargs);

  pdr::Results rs(model);
  pdr::PDR algorithm(context, logger, clargs.max_pebbles);

  if (clargs.tactic == pdr::Tactic::basic)
  {
    pdr::Result r = algorithm.run(clargs.tactic, clargs.max_pebbles);
    rs.add(r).show(strategy);
    algorithm.show_solver(solver_dump);

    return 0;
  }
  else
  {
    pdr::pebbling::Optimizer optimize(std::move(algorithm));
    std::optional<unsigned> optimum = optimize.run(clargs);
    optimize.latest_results.show(strategy);
    optimize.dump_solver(solver_dump);
  }

  std::cout << "run done" << std::endl;
  return 0;
}
