#include "dag.h"
#include "h-operator.h"
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

// const fs::path BENCH_FOLDER = fs::current_path() / "benchmark" / "rls" /
// "tfc";
const fs::path BENCH_FOLDER = fs::current_path();

struct hop_arg
{
  unsigned bits;
  unsigned mod;
};

enum ModelType
{
  none,
  tfc,
  bench,
  hoperator
};

struct ArgumentList
{
  OutLvl verbosity;
  ModelType model;
  std::string model_name;
  hop_arg hop;
  fs::path bench_folder;

  int max_pebbles;

  // run options
  bool rand;
  bool delta;
  bool onlyshow;
  pdr::Run pdr_type;

  bool _failed = false;
};

const std::string decrement_str("dec");
const std::string increment_str("inc");
std::string run_str(pdr::Run r)
{
  switch (r)
  {
    case pdr::Run::basic: return "basic";
    case pdr::Run::decrement: return decrement_str;
    case pdr::Run::increment: return increment_str;
    default: throw std::invalid_argument("pdr::Run is undefined");
  }
}

// FILE IO
//
std::string file_name(const ArgumentList& args)
{
  std::string pebble_str;
  if (args.pdr_type == pdr::Run::basic)
    pebble_str = "-" + std::to_string(args.max_pebbles);
  else
    pebble_str = "";

  return fmt::format("{}-{}{}{}", args.model_name, run_str(args.pdr_type),
                     pebble_str, args.delta ? "-delta" : "");
}

std::string folder_name(const ArgumentList& args)
{
  std::string pebble_str;
  if (args.pdr_type == pdr::Run::basic)
    pebble_str = "-" + std::to_string(args.max_pebbles);
  else
    pebble_str = "";

  return fmt::format("{}{}{}", run_str(args.pdr_type), pebble_str,
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
  clargs.delta = false;
  // clang-format off
  clopt.add_options()
    ("v,verbose", "Output all messages during pdr iterations",
      cxxopts::value<bool>()->default_value("false"))
    ("w,wisper", "Output only minor messages during pdr iterations.",
      cxxopts::value<bool>()->default_value("false"))
    ("showonly", "Only write the given model to its output file, does not run the algorithm.",
     cxxopts::value<bool>(clargs.onlyshow))

    ("optimize", fmt::format("Find a strategy requiring the minimum number of pebbles."
                             "Value is {} or {}", increment_str, decrement_str), 
      cxxopts::value<std::string>(), "(string:TYPE)")
    ("d,delta", "Use delta-encoded frames.",
      cxxopts::value<bool>(clargs.delta))
    ("r,randomize", "Use a randomized seed for the SAT solver",
      cxxopts::value<bool>(clargs.rand))

    ("dir","Directory (relative to ./) than contains runable benchmarks.",
      cxxopts::value<fs::path>()->default_value(BENCH_FOLDER), "(string:F)")
    ("bench", "File in in .bench format.",
      cxxopts::value<std::string>(), "(string:FILE)")
    ("tfc", "File in in .tfc format.",
      cxxopts::value<std::string>(), "(string:FILE)")
    ("hop", "Construct h-operator model from provided bitwidth (BITS) and modulus (MOD).",
      cxxopts::value<std::vector<unsigned>>(), "(uint:BITS,uint:MOD)")
    ("p,pebbles", "The number of strategy's to find a strategy for."
     "Ignored during optimize runs.", 
      cxxopts::value<int>(clargs.max_pebbles), "(uint:N)")


    ("h,help", "Show usage");
  // clang-format on

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

    if (clresult.count("verbose"))
      clargs.verbosity = OutLvl::verbose;
    else if (clresult.count("whisper"))
      clargs.verbosity = OutLvl::whisper;
    else
      clargs.verbosity = OutLvl::silent;

    if (clresult.count("optimize"))
    {
      std::string tactic = clresult["optimize"].as<std::string>();
      if (tactic == "inc")
        clargs.pdr_type = pdr::Run::increment;
      else if (tactic == "dec")
        clargs.pdr_type = pdr::Run::decrement;
      else
        throw std::invalid_argument(fmt::format(
            "optimize must be either {} or {}.", increment_str, decrement_str));
    }
    else
      clargs.pdr_type = pdr::Run::basic;

    // read { model | hop }
    unsigned n_models =
        clresult.count("hop") + clresult.count("tfc") + clresult.count("bench");
    if (n_models != 1)
      throw std::invalid_argument("specify one of tfc, bench, or hop.");

    if (clresult.count("hop"))
    {
      clargs.model = ModelType::hoperator;

      const auto hop_list = clresult["hop"].as<std::vector<unsigned>>();
      if (hop_list.size() != 2)
        throw std::invalid_argument(
            "hop requires two positive integers: bitwidth,modulus");
      clargs.hop        = { hop_list[0], hop_list[1] };
      clargs.model_name = fmt::format("hop{}_{}", hop_list[0], hop_list[1]);
    }

    if (clresult.count("tfc"))
    {
      clargs.model      = ModelType::tfc;
      clargs.model_name = clresult["tfc"].as<std::string>();
    }

    if (clresult.count("bench"))
    {
      clargs.model      = ModelType::bench;
      clargs.model_name = clresult["bench"].as<std::string>();
    }

    // read starting no. pebbles
    if (clresult.count("pebbles"))
    {
      clargs.max_pebbles = clresult["pebbles"].as<int>();
      if (clargs.max_pebbles < 1)
        throw std::invalid_argument("pebbles must be greater than 0.");
    }
    else // begin
      clargs.max_pebbles = -1;

    clargs.bench_folder = BENCH_FOLDER / clresult["dir"].as<fs::path>();
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
            << std::endl;
  if (clargs.pdr_type != pdr::Run::basic)
    std::cout << fmt::format("Using finding minimal cardinality ({}). ",
                             run_str(clargs.pdr_type));
  if (clargs.delta)
    std::cout << "Using delta-encoded frames.";
  std::cout << std::endl;
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
      auto const result = lorina::read_bench(model_file.string(),
                                             mockturtle::bench_reader(klut));
      if (result != lorina::return_code::success)
        throw std::invalid_argument(model_file.string() +
                                    " is not a valid .bench file");

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
  const auto [model_dir, base_dir] = output_paths(clargs);
  std::string filename             = file_name(clargs);

  std::ofstream graph_descr = trunc_file(model_dir, "graph", "txt");
  std::ofstream model_descr = trunc_file(model_dir, "model", "txt");

  // build directed acyclic graph
  dag::Graph G = build_dag(clargs);
  G.show_image(model_dir / "dag");
  std::cout << G.summary() << std::endl;
  graph_descr << G.summary() << std::endl << G;

  // create model from DAG graph and set up algorithm
  if (clargs.max_pebbles < 1)
    clargs.max_pebbles = G.nodes.size();

  z3::config ctx_settings;
  ctx_settings.set("unsat_core", true);
  ctx_settings.set("model", true);
  pdr::Model model(ctx_settings, clargs.model_name, G, clargs.max_pebbles);
  model.show(model_descr);

  pdr::context context(model, clargs.delta, clargs.rand);

  if (clargs.onlyshow)
    return 0;

  std::ofstream stats       = trunc_file(base_dir, filename, "stats");
  std::ofstream strategy    = trunc_file(base_dir, filename, "strategy");
  std::ofstream solver_dump = trunc_file(base_dir, "solver_dump", "strategy");

  // initialize logger and other bookkeeping
  fs::path log_file      = base_dir / fmt::format("{}.{}", filename, "log");
  fs::path progress_file = base_dir / fmt::format("{}.{}", filename, "out");

  pdr::Logger pdr_logger(log_file.string(), G, progress_file.string(),
                         clargs.verbosity);
  pdr::Results res(model);

  // run pdr and write output
  show_header(clargs);

  pdr::PDR algorithm(context, pdr_logger, res);

  if (clargs.pdr_type == pdr::Run::decrement)
    algorithm.decrement_strategy(strategy, solver_dump);

  if (clargs.pdr_type == pdr::Run::increment)
    algorithm.increment_strategy(strategy, solver_dump);

  if (clargs.pdr_type == pdr::Run::basic)
  {
    algorithm.run();
    algorithm.show_results(strategy);
    algorithm.show_solver(solver_dump);

  }
  // while (true)
  // {
  //   bool found_strategy = !algorithm.run(clargs.opt);
  //   stats << "Cardinality: " << model.get_max_pebbles() << std::endl;
  //   stats << pdr_logger.stats << std::endl;

  //   if (!found_strategy || clargs.one)
  //     break;

  //   if (algorithm.decrement(true))
  //     break;
  // }
  // algorithm.show_results(strategy);
  // solver_dump << SEP3 << " iteration " << clargs.max_pebbles << std::endl;
  // algorithm.show_solver(solver_dump);

  return 0;
}
