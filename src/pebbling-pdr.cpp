#include "dag.h"
#include "h-operator.h"
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
#include <fmt/format.h>
#include <fstream>
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

// FILE IO
//
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

std::array<fs::path, 2> paths(const std::string& model_name, unsigned n_pebbles,
                              bool optimize, bool delta)
{
  fs::path results_folder = fs::current_path() / "output";
  std::string opts        = folder_name(n_pebbles, optimize, delta);
  fs::create_directory(results_folder);
  fs::create_directory(results_folder / model_name);
  fs::create_directory(results_folder / model_name / opts);

  return { results_folder / model_name, results_folder / model_name / opts };
}

std::ofstream trunc_file(const fs::path& folder, const std::string& filename,
                         const std::string& ext)
{
  fs::path file = folder / fmt::format("{}.{}", filename, ext);
  std::ofstream stream(file.string(), std::fstream::out | std::fstream::trunc);
  assert(stream.is_open());
  return stream;
}
//
// FILE IO

// ensure proper folders exist and create file names for In and Output
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

  // bench model
  // fs::path bench_folder =
  //     fs::current_path() / "benchmark" / "iscas85" / "bench";
  // fs::path model_file = fs::current_path() /
  //                                    "benchmark" / "iscas85" / "bench" /
  //                                    (clargs.model_name + ".bench");

  // tfc model
  fs::path model_file = clargs.bench_folder / (clargs.model_name + ".tfc");

  const auto [model_dir, base_dir] = paths(
      clargs.model_name, clargs.max_pebbles, clargs.optimize, clargs.delta);
  std::string filename = file_name(clargs.model_name, clargs.max_pebbles,
                                   clargs.optimize, clargs.delta);

  std::ofstream stats       = trunc_file(base_dir, filename, "stats");
  std::ofstream strategy    = trunc_file(base_dir, filename, "strategy");
  std::ofstream solver_dump = trunc_file(base_dir, "solver_dump", "strategy");
  std::ofstream model_descr = trunc_file(model_dir, "model", "txt");

  // read input model
  dag::Graph G       = dag::hoperator(2, 3);
  clargs.max_pebbles = G.nodes.size();
  std::cout << fmt::format("Graph {{ In: {}, Out {}, Nodes {} }}",
                           G.input.size(), G.output.size(), G.nodes.size())
            << std::endl;
  model_descr << G;
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
  fs::path log_file = base_dir / fmt::format("{}.{}", filename, ".log");
  fs::path progress_file = base_dir / fmt::format("{}.{}", filename, ".log");

  pdr::Logger pdr_logger(log_file.string(), G, progress_file.string(),
                         OutLvl::verbose);
  pdr::PDResults res(model);

  std::cout << std::endl
            << fmt::format("Finding {}-pebble strategy for {}",
                           clargs.max_pebbles, clargs.model_name)
            << std::endl
            << (clargs.optimize ? "Using dynamic cardinality. " : "")
            << (clargs.delta ? "Using delta-encoded frames." : "") << std::endl;

  // run pdr and write output
  if (clargs.optimize)
  {
    pdr::PDR algorithm(model, clargs.delta, pdr_logger, res);

    while (true)
    {
      bool found_strategy = !algorithm.run(clargs.optimize);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      if (!found_strategy)
        break;

      if (algorithm.decrement(true))
        break;
    }
    algorithm.show_results(strategy);
    algorithm.show_solver(solver_dump, clargs.max_pebbles);
  }
  else
  {
    // TODO multiple normal runs from comparision
    while (true)
    {
      pdr::PDR algorithm(model, clargs.delta, pdr_logger, res);
      bool found_strategy = !algorithm.run(clargs.optimize);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      algorithm.show_solver(solver_dump, model.get_max_pebbles());

      if (!found_strategy && algorithm.decrement(true))
      {
        algorithm.show_results(strategy);
        break;
      }
    }
  }
  return 0;
}
