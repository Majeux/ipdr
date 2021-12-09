#include "dag.h"
#include "h-operator.h"
#include "logger.h"
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

struct hop_arg
{
  unsigned bits;
  unsigned mod;
};

enum ModelType
{
  none,
  file,
  hoperator
};

struct ArgumentList
{
  bool verbose;
  bool whisper;

  ModelType model;
  std::string model_name;
  hop_arg hop;
  fs::path bench_folder;

  int max_pebbles;

  // run options
  bool opt;
  bool delta;
  bool onlyshow;
  bool one;

  bool _failed = false;
};

// FILE IO
//
// "{model}-{n pebbles}[_opt?][_delta?]"
std::string file_name(const ArgumentList& args)
{
  return fmt::format("{}-{}pebbles{}{}", args.model_name, args.max_pebbles,
                     args.opt ? "_opt" : "", args.delta ? "_delta" : "");
}

std::string folder_name(const ArgumentList& args)
{
  return fmt::format("{}{}{}", args.max_pebbles, args.opt ? "-opt" : "",
                     args.delta ? "-delta" : "");
}

std::array<fs::path, 2> output_paths(const ArgumentList& args)
{
  fs::path results_folder = fs::current_path() / "output";
  std::string opts        = folder_name(args);
  fs::create_directory(results_folder);
  fs::path model_dir = results_folder / args.model_name;
  fs::create_directory(model_dir);
  fs::path run_dir = results_folder / args.model_name / opts;
  fs::create_directory(run_dir);

  return { model_dir, run_dir };
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
// end FILE IO

// CLI
//
cxxopts::Options make_options(std::string name, ArgumentList& clargs)
{
  cxxopts::Options clopt(name, "Find a pebbling strategy using a minumum "
                               "amount of pebbles through PDR");
  clargs.opt = clargs.delta = false;
  // clang-format off
  clopt.add_options()
    ("v,verbose", "Remove all output during pdr iterations",
      cxxopts::value<bool>(clargs.verbose)->default_value("false"))
    ("w,wisper", "Output only minor info during pdr iterations.",
      cxxopts::value<bool>(clargs.verbose)->default_value("false"))
    ("showonly", "Only write the given model to its output file, does not run the algorithm.",
     cxxopts::value<bool>(clargs.onlyshow))

    ("o,optimize", "Multiple runs that find a strategy with minimum pebbles.",
      cxxopts::value<bool>(clargs.opt))
    ("d,delta", "Use delta-encoded frames.",
      cxxopts::value<bool>(clargs.delta))
    ("one", "Only run one iteration of pdr, which verifies if there is a strategy for the number of pebbles.",
      cxxopts::value<bool>(clargs.one))

    ("model", "Name of the graph to pebble.",
      cxxopts::value<std::string>(clargs.model_name), "string:NAME")
    ("hop", "Construct h-operator model from provided bitwidth (BITS) and modulus (MOD).",
      cxxopts::value<std::vector<unsigned>>(), "uint:BITS,uint:MOD")
    ("p,pebbles", "Starting maximum number of pebbles for the strategy search"
      "Defaults to the highest possible if omitted.",
      cxxopts::value<int>(clargs.max_pebbles), "uint:N")

    ("b,benchfolder","Folder than contains runable .tfc benchmarks.",
      cxxopts::value<fs::path>()->default_value(BENCH_FOLDER), "string:F")

    ("h,help", "Show usage");
  // clang-format on
  // clopt.parse_positional({ "pebbles" });
  // clopt.positional_help("<max pebbles>").show_positional_help();

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

    // read { model | hop }
    clargs.model = ModelType::none;
    if (clresult.count("model"))
    {
      clargs.model      = ModelType::file;
      clargs.model_name = clresult["model"].as<std::string>();
    }

    if (clresult.count("hop"))
    {
      if (clargs.model == ModelType::file)
        throw std::invalid_argument("specify either { model | hop }, not both");
      clargs.model = ModelType::hoperator;

      const auto hop_list = clresult["hop"].as<std::vector<unsigned>>();
      if (hop_list.size() != 2)
        throw std::invalid_argument(
            "hop requires two positive integers: bitwidth,modulus");
      clargs.hop.bits   = hop_list[0];
      clargs.hop.mod    = hop_list[1];
      clargs.model_name = fmt::format("hop{}_{}", hop_list[0], hop_list[1]);
    }

    if (clargs.model == ModelType::none)
      throw std::invalid_argument("specify either { model | hop }");

    // read no pebbles
    if (clresult.count("pebbles"))
    {
      clargs.max_pebbles = clresult["pebbles"].as<int>();
      if (clargs.max_pebbles == 0)
        throw new std::invalid_argument("pebbles must be greater than 0.");
    }
    else
      clargs.max_pebbles = -1;

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
//
// end CLI

// OUTPUT
////
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

void show_header(const ArgumentList& clargs)
{
  std::cout << std::endl
            << fmt::format("Finding {}-pebble strategy for {}",
                           clargs.max_pebbles, clargs.model_name)
            << std::endl
            << (clargs.opt ? "Using dynamic cardinality. " : "")
            << (clargs.delta ? "Using delta-encoded frames." : "") << std::endl;
}
//
// end OUTPUT

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  ArgumentList clargs = parse_cl(argc, argv);

  const auto [model_dir, base_dir] = output_paths(clargs);
  // clargs.model_name, clargs.max_pebbles, clargs.optimize, clargs.delta);
  std::string filename             = file_name(clargs);

  std::ofstream graph_descr = trunc_file(model_dir, "graph", "txt");
  std::ofstream model_descr = trunc_file(model_dir, "model", "txt");
  // read input model
  dag::Graph G;
  switch (clargs.model)
  {
    case ModelType::hoperator:
    {
      G = dag::hoperator(clargs.hop.bits, clargs.hop.mod);
    }
    break;
    case ModelType::file:
    {
      parse::TFCParser parser;
      fs::path model_file = clargs.bench_folder / (clargs.model_name + ".tfc");
      G = parser.parse_file(model_file.string(), clargs.model_name);
    }
    break;
    default: break;
  }
  G.show_image(model_dir / "dag");
  std::cout << G.summary() << std::endl;
  graph_descr << G.summary() << std::endl << G;

  // create model from DAG graph and set up algorithm
  if (clargs.max_pebbles < 0)
    clargs.max_pebbles = G.nodes.size();

  PDRModel model;
  model.load_model(clargs.model_name, G, clargs.max_pebbles);
  model.show(model_descr);

  if (clargs.onlyshow)
    return 0;

  std::ofstream stats       = trunc_file(base_dir, filename, "stats");
  std::ofstream strategy    = trunc_file(base_dir, filename, "strategy");
  std::ofstream solver_dump = trunc_file(base_dir, "solver_dump", "strategy");

  // initialize logger and other bookkeeping
  fs::path log_file      = base_dir / fmt::format("{}.{}", filename, ".log");
  fs::path progress_file = base_dir / fmt::format("{}.{}", filename, ".log");

  pdr::Logger pdr_logger(log_file.string(), G, progress_file.string(),
                         OutLvl::verbose);
  pdr::PDResults res(model);

  // run pdr and write output
  show_header(clargs);
  if (clargs.opt)
  {
    pdr::PDR algorithm(model, clargs.delta, pdr_logger, res);

    while (true)
    {
      bool found_strategy = !algorithm.run(clargs.opt);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      if (!found_strategy || clargs.one)
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
      bool found_strategy = !algorithm.run(clargs.opt);
      stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
      stats << pdr_logger.stats << std::endl;

      algorithm.show_solver(solver_dump, model.get_max_pebbles());

      if (clargs.one)
        break;

      if (!found_strategy && algorithm.decrement(true))
      {
        algorithm.show_results(strategy);
        break;
      }
    }
  }
  return 0;
}
