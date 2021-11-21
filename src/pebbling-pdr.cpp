#include "dag.h"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "pdr-model.h"
#include "pdr.h"

#include <array>
#include <cassert>
#include <cstring>
#include <cxxopts.hpp>
#include <exception>
#include <fmt/core.h>
#include <fstream>
// #include <filesystem>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <z3++.h>

namespace fs = ghc::filesystem;

const fs::path BENCH_FOLDER = fs::current_path() / "benchmark" / "rls" / "tfc";

// "{model}-{n pebbles}[_opt?][_delta?]"
std::string file_name(const std::string& name, unsigned n, bool opt, bool delta)
{
  return fmt::format("{}-{}pebbles{}{}", name, n, opt ? "_opt" : "",
                     delta ? "_delta" : "");
}

std::string folder_name(unsigned n, bool opt, bool delta)
{
  return fmt::format("{}{}{}", n, opt ? "-opt" : "", delta ? "-delta" : "");
}

// ensure proper folders exist and create file names for In and Output
std::array<std::string, 5> setup_in_out(const std::string& model_name,
                                        unsigned n_pebbles, bool optimize,
                                        bool delta)
{
  fs::path results_folder = fs::current_path() / "output";

  std::string opts = folder_name(n_pebbles, optimize, delta);
  fs::create_directory(results_folder);
  fs::create_directory(results_folder / model_name);
  fs::create_directory(results_folder / model_name / opts);
  fs::path base = results_folder / model_name / opts;

  fs::path stats_file =
      base / (file_name(model_name, n_pebbles, optimize, delta) + ".stats");
  fs::path strategy_file =
      base / (file_name(model_name, n_pebbles, optimize, delta) + ".strategy");
  fs::path log_file =
      base / (file_name(model_name, n_pebbles, optimize, delta) + ".log");
  fs::path solver_file = base / "solver_dump.solver";
  fs::path progress    = base / "progress.txt";

  // show used paths
  TextTable output_files;
  auto cwd_row  = { std::string(" current dir "), fs::current_path().string() };
  auto stat_row = { std::string(" stats file "), stats_file.string() };
  auto res_row  = { std::string(" result file "), strategy_file.string() };
  auto solver_row = { std::string(" solver_dump file"), solver_file.string() };
  output_files.addRow(cwd_row);
  output_files.addRow(stat_row);
  output_files.addRow(res_row);
  output_files.addRow(solver_row);
  std::cout << output_files << std::endl;

  return { stats_file.string(), strategy_file.string(), log_file.string(),
           solver_file.string(), progress.string() };
}

struct ArgumentList
{
  bool verbose;
  bool whisper;
  std::string model_name;
  fs::path bench_folder;
  unsigned max_pebbles;
  bool optimize;
  bool delta;

  bool _failed = false;
};

cxxopts::Options make_options(std::string name, ArgumentList& clargs)
{
  cxxopts::Options clopt(name, "Find a pebbling strategy using a minumum "
                               "amount of pebbles through PDR");
  clargs.optimize = clargs.delta = false;
  // clang-format off
  clopt.add_options()
    ("v,verbose", "Remove all output during pdr iterations",
      cxxopts::value<bool>(clargs.verbose)->default_value("false"))
    ("w,wisper", "Output only minor info during pdr iterations",
      cxxopts::value<bool>(clargs.verbose)->default_value("false"))
    ("o, optimize", "Multiple runs that find a strategy with minimum pebbles",
      cxxopts::value<bool>(clargs.optimize))
    ("d, delta", "Use delta-encoded frames",
      cxxopts::value<bool>(clargs.delta))
    ("model", "Name of the graph to pebble",
      cxxopts::value<std::string>(clargs.model_name))
    ("pebbles", "Maximum number of pebbles for a strategy",
      cxxopts::value<unsigned>(clargs.max_pebbles))
    ("b,benchfolder","Folder than contains runable .tfc benchmarks",
      cxxopts::value<fs::path>()->default_value(BENCH_FOLDER))
    ("h,help", "Show usage");
  // clang-format on
  clopt.parse_positional({ "model", "pebbles" });
  clopt.positional_help("<model name> <max pebbles>").show_positional_help();

  return clopt;
}

ArgumentList parse_cl(int argc, char* argv[])
{
  ArgumentList clargs;
  cxxopts::Options clopt = make_options(argv[0], clargs);
  try
  {
    auto clresult = clopt.parse(argc, argv);

    if (clresult.count("help"))
    {
      std::cerr << clopt.help() << std::endl;
      exit(0);
    }

    if (clresult.count("model"))
      clargs.model_name = clresult["model"].as<std::string>();
    else
      throw std::invalid_argument("<model name> is required");

    if (clresult.count("pebbles"))
      clargs.max_pebbles = clresult["pebbles"].as<unsigned>();
    else
      throw std::invalid_argument("<max pebbles> is required");

    clargs.bench_folder = BENCH_FOLDER / clresult["benchfolder"].as<fs::path>();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error parsing command line arguments" << std::endl
              << std::endl;
    std::cerr << clopt.help() << std::endl;
    throw;
  }

  return clargs;
}

int main(int argc, char* argv[])
{
  ArgumentList clargs = parse_cl(argc, argv);

  std::cout << fmt::format("Finding {}-pebble strategy for {}",
                           clargs.max_pebbles, clargs.model_name)
            << std::endl
            << (clargs.optimize ? "Using dynamic cardinality. " : "")
            << (clargs.delta ? "Using delta-encoded frames." : "") << std::endl;

  // bench model
  // fs::path bench_folder =
  //     fs::current_path() / "benchmark" / "iscas85" / "bench";
  // fs::path model_file = fs::current_path() /
  //                                    "benchmark" / "iscas85" / "bench" /
  //                                    (clargs.model_name + ".bench");

  // tfc model
  fs::path model_file = clargs.bench_folder / (clargs.model_name + ".tfc");

  const auto [stats_file, strategy_file, log_file, solver_file, progress_file] =
      setup_in_out(clargs.model_name, clargs.max_pebbles, clargs.optimize,
                   clargs.delta);

  std::ofstream stats(stats_file, std::fstream::out | std::fstream::trunc);
  std::ofstream results(strategy_file, std::fstream::out | std::fstream::trunc);
  std::ofstream solver_dump(solver_file,
                            std::fstream::out | std::fstream::trunc);

  assert(stats.is_open());
  assert(results.is_open());

  // read input model
  parse::TFCParser parser;
  dag::Graph G = parser.parse_file(model_file, clargs.model_name);
  // dag::Graph G = parse::parse_bench(model_file, clargs.model_name);

  std::cout << "Graph" << std::endl << G;
  // G.export_digraph(BENCH_FOLDER.string());

  // init z3
  z3::config settings;
  settings.set("unsat_core", true);
  settings.set("model", true);

  // create model from DAG graph and set up algorithm
  PDRModel model(settings);
  model.load_model(clargs.model_name, G, clargs.max_pebbles);
  model.show(std::cout);
  // initialize logger and other bookkeeping
  pdr::Logger pdr_logger(log_file, G, progress_file, OutLvl::verbose);
  pdr::PDResults res(model);

  // run pdr and write output
  if (clargs.optimize)
  {
    pdr::PDR algorithm(model, clargs.delta, pdr_logger, res);

    while (true)
    {
      bool strategy = !algorithm.run(clargs.optimize);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      if (!strategy)
        break;

      if (algorithm.decrement(true))
        break;
    }
    algorithm.show_results(results);
    algorithm.show_solver(solver_dump, clargs.max_pebbles);
  }
  else
  {
    // TODO multiple normal runs from comparision
    while (true)
    {
      pdr::PDR algorithm(model, clargs.delta, pdr_logger, res);
      bool strategy = !algorithm.run(clargs.optimize);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      algorithm.show_solver(solver_dump, model.get_max_pebbles());

      if (!strategy && algorithm.decrement(true))
      {
        algorithm.show_results(results);
        break;
      }
    }
  }
  return 0;
}
