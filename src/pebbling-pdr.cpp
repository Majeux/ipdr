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

namespace fs = ghc::filesystem;
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

int main(int argc, char* argv[])
{
  ArgumentList clargs = parse_cl(argc, argv);

  // create files for I/O
  static fs::path model_dir        = setup_model_path(clargs);
  static std::ofstream graph_descr = trunc_file(model_dir, "graph", "txt");
  static std::ofstream model_descr = trunc_file(model_dir, "model", "txt");
  static fs::path run_dir          = setup_run_path(clargs);

  dag::Graph G = build_dag(clargs);
  G.show_image(model_dir / "dag");
  std::cout << G.summary() << std::endl;
  graph_descr << G.summary() << std::endl << G;

  static z3::config ctx_settings;
  ctx_settings.set("unsat_core", true);
  ctx_settings.set("model", true);
  pdr::pebbling::Model model(ctx_settings, clargs, G);
  model.show(model_descr);

  if (clargs.onlyshow)
    return 0;

  pdr::Context context = clargs.seed
                           ? pdr::Context(model, clargs.delta, *clargs.seed)
                           : pdr::Context(model, clargs.delta, clargs.rand);

  const std::string filename = file_name(clargs);
  std::ofstream stats        = trunc_file(run_dir, filename, "stats");
  std::ofstream strategy     = trunc_file(run_dir, filename, "strategy");
  std::ofstream solver_dump  = trunc_file(run_dir, "solver_dump", "strategy");

  // initialize logger and other bookkeeping
  fs::path log_file      = run_dir / fmt::format("{}.{}", filename, "log");
  fs::path progress_file = run_dir / fmt::format("{}.{}", filename, "out");

  pdr::Logger pdr_logger = clargs.out
                             ? pdr::Logger(log_file.string(), G, *clargs.out,
                                   clargs.verbosity, std::move(stats))
                             : pdr::Logger(log_file.string(), G,
                                   clargs.verbosity, std::move(stats));

  pdr::PDR algorithm(context, pdr_logger);

  show_header(clargs);

  switch (clargs.tactic)
  {
    case pdr::Tactic::decrement:
      algorithm.dec_tactic(strategy, solver_dump);
      break;
    case pdr::Tactic::increment:
      algorithm.inc_tactic(strategy, solver_dump);
      break;
    case pdr::Tactic::basic:
      algorithm.run();
      // algorithm.show_results(strategy);
      algorithm.show_solver(solver_dump);
      break;
    case pdr::Tactic::inc_jump_test:
      algorithm.inc_jump_test(
          clargs.max_pebbles.value(), 10, strategy, solver_dump);
      break;
    case pdr::Tactic::inc_one_test:
      algorithm.inc_jump_test(
          clargs.max_pebbles.value(), 1, strategy, solver_dump);
      break;
    default: throw std::invalid_argument("No pdr tactic has been selected.");
  }
  std::cout << "done" << std::endl;
  return 0;
}
