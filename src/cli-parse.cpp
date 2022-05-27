#include "cli-parse.h"
#include "io.h"
#include "logger.h"
#include "tactic.h"
#include <cxxopts.hpp>

#warning display settings for: delta, seed, mic_retries
namespace my::cli
{
  namespace fs = ghc::filesystem;
  using namespace pdr::tactic;

  void show_header(const ArgumentList& clargs)
  {
    using fmt::format;
    using std::cout;
    using std::endl;
    switch (clargs.tactic)
    {
      case pdr::Tactic::basic:
        cout << format("Finding {}-pebble strategy for {}",
            clargs.max_pebbles.value(), clargs.model_name);
        break;
      case pdr::Tactic::decrement:
        cout << format("Finding minimal pebble strategy for {} by decrementing",
            clargs.model_name);
        break;
      case pdr::Tactic::increment:
        cout << format("Finding minimal pebble strategy for {} by incrementing",
            clargs.model_name);
        break;
      case pdr::Tactic::inc_jump_test:
        cout << format("{} and +10 step jump test for {} by incrementing",
            clargs.max_pebbles.value(), clargs.model_name);
        break;
      case pdr::Tactic::inc_one_test:
        cout << format("{} and +1 step jump test for {} by incrementing",
            clargs.max_pebbles.value(), clargs.model_name);
        break;
      default: throw std::invalid_argument("pdr::Tactic is undefined");
    }
    cout << endl;
    if (clargs.exp_sample)
      cout << format(
          "Running an experiment with {} samples. ", *clargs.exp_sample);
    if (clargs.experiment_control)
      cout << "(a control run)";
    cout << endl;

    if (clargs.delta)
      cout << "Using delta-encoded frames." << endl;
    if (clargs.rand)
      cout << "Using randomized seed." << endl;
    if (clargs.seed)
      cout << "Using seed: " << *clargs.seed << endl;
    if (clargs.tseytin)
      cout << "Using tseytin encoded transition." << endl;
    cout << endl;
  }

  cxxopts::Options make_options(std::string name, ArgumentList& clargs)
  {
    using cxxopts::value;
    using std::optional;
    using std::string;
    using std::vector;

    cxxopts::Options clopt(name, "Find a pebbling strategy using a minumum "
                                 "amount of pebbles through PDR");
    // clang-format off
    clopt.add_options()
      // output options
      ("v,verbose", "Output all messages during pdr iterations")
      ("w,wisper", "Output only minor messages during pdr iterations. (default)")
      ("s,silent", "Output no messages during pdr iterations.")
      ("out-file", "Write to an output file instead of standard output", 
        value< optional<string> >(clargs.out), "(string:FILE.out)")

      // tactic option
      ("optimize", fmt::format("Find a strategy requiring the minimum number of pebbles."
       "Value is {} or {}", increment_str, decrement_str), 
        value<string>(), "(string:TYPE)")
      ("e,experiment", "Run an for a given optimization tactic.",
        value< optional<unsigned> >(clargs.exp_sample))
      ("control", "Run an optimization tactic with only multiple Basic runs",
        value<bool>(clargs.experiment_control))

      // model options
      ("dir","Directory (relative to ./) than contains runable benchmarks.",
        value<fs::path>()->default_value(my::io::BENCH_FOLDER), "(string:F)")
      ("bench", "File in in .bench format.",
        value<string>(), "(string:FILE)")
      ("tfc", "File in in .tfc format.",
        value<string>(), "(string:FILE)")
      ("hop", "Construct h-operator model from provided bitwidth (BITS) and modulus (MOD).",
        value< vector<unsigned> >(), "(uint:BITS,uint:MOD)")
      ("p,pebbles", "The number of strategy's to find a strategy for."
       "Ignored during optimize runs.", 
        value< optional<unsigned> >(clargs.max_pebbles), "(uint:N)")

      // context options
      ("r,rand", "Use a randomized seed for the SAT solver",
        value<bool>(clargs.rand))
      ("seed", "Use the given seed for the SAT solver",
        value< optional<unsigned> >(clargs.seed), "(uint:SEED)")
      ("d,delta", "Use delta-encoded frames.",
        value<bool>(clargs.delta))
      ("tseytin", "Build the transition relation using the tseytin reform.",
        value<bool>(clargs.tseytin))

      // tests
      (inc_jump_str, "Test two runs: one with pebbles 10 higher than the other.",
       value<unsigned>(), "(uint:P)")
      (inc_one_str, "Test two runs: one with P pebbles and the other with P+1.",
       value<unsigned>(), "(uint:P)")
      ("bounded", "Run pebbling model using bounded model checking",
       value<bool>(clargs.bounded))

      ("show-only", "Only write the given model to its output file, does not run the algorithm.",
       value<bool>(clargs.onlyshow))

    ("h,help", "Show usage");
    // clang-format on
#warning add mic_retries option
    return clopt;
  }

  void parse_verbosity(
      ArgumentList& clargs, const cxxopts::ParseResult& clresult)
  {
    if (clresult.count("verbose"))
      clargs.verbosity = OutLvl::verbose;
    else if (clresult.count("whisper"))
      clargs.verbosity = OutLvl::whisper;
    else if (clresult.count("silent"))
      clargs.verbosity = OutLvl::silent;
    else
      clargs.verbosity = OutLvl::whisper;
  }

  void parse_tactic(ArgumentList& clargs, const cxxopts::ParseResult& clresult)
  {
    if (clresult.count("bounded"))
    {
      clargs.tactic = pdr::Tactic::decrement;
    }
    if (clresult.count("optimize"))
    {
      std::string tactic = clresult["optimize"].as<std::string>();
      if (tactic == "inc")
        clargs.tactic = pdr::Tactic::increment;
      else if (tactic == "dec")
        clargs.tactic = pdr::Tactic::decrement;
      else
        throw std::invalid_argument(fmt::format(
            "optimize must be either {} or {}.", increment_str, decrement_str));
    }
    else if (clresult.count(inc_jump_str))
    {
      clargs.tactic      = pdr::Tactic::inc_jump_test;
      clargs.max_pebbles = clresult[inc_jump_str].as<unsigned>();
    }
    else if (clresult.count(inc_one_str))
    {
      clargs.tactic      = pdr::Tactic::inc_one_test;
      clargs.max_pebbles = clresult[inc_one_str].as<unsigned>();
    }
    else
    {
      clargs.tactic = pdr::Tactic::basic;
      if (!clresult.count("bounded"))
        if (!clresult.count("pebbles"))
          throw std::invalid_argument("Basic run requires a \"pebbles\" value.");

      // clargs.max_pebbles = clresult["pebbles"].as<unsigned>();
      // if (clargs.max_pebbles < 0)
      //   throw std::invalid_argument("pebbles must be positive.");
    }

    unsigned n_tests =
        clresult.count(pdr::tactic::to_string(pdr::Tactic::inc_jump_test)) +
        clresult.count(pdr::tactic::to_string(pdr::Tactic::inc_one_test));
    if (n_tests > 1)
      throw std::invalid_argument("specify at most one test");
  }

  void parse_model(ArgumentList& clargs, const cxxopts::ParseResult& clresult)
  {
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

    clargs.bench_folder = my::io::BENCH_FOLDER / clresult["dir"].as<fs::path>();
  }

  ArgumentList parse_cl(int argc, char* argv[])
  {
    ArgumentList clargs;
    cxxopts::Options clopt = make_options(argv[0], clargs);
    try
    {
      cxxopts::ParseResult clresult = clopt.parse(argc, argv);

      if (clresult.count("help"))
      {
        std::cerr << clopt.help() << std::endl;
        exit(0);
      }

      parse_verbosity(clargs, clresult);
      parse_model(clargs, clresult);
      if (clargs.onlyshow)
        return clargs;
      parse_tactic(clargs, clresult);
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
} // namespace my::cli
