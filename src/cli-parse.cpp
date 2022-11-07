#include "cli-parse.h"
#include "io.h"
#include "logger.h"
#include "tactic.h"
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <initializer_list>
#include <numeric>
#include <ostream>
#include <stdexcept>

#warning display settings for: delta, seed, mic_retries
namespace my::cli
{
  namespace fs = ghc::filesystem;
  using namespace pdr::tactic;
  using fmt::format;
  using std::string;

  // STRUCTS
  //
  namespace model_type
  {
    std::string model_t_string_visitor::operator()(const Pebbling& m) const
    {
      if (m.max_pebbles)
        return format("Pebbling algorithm. {} pebbles.", *m.max_pebbles);
      else
        return "Pebbling algorithm.";
    }

    std::string model_t_string_visitor::operator()(const Peterson& m) const
    {
      return format(
          "Peterson algorithm. {} processes out of {} max.", m.start, m.max);
    }

    std::string to_string(const std::variant<Pebbling, Peterson>& m)
    {
      return std::visit(model_t_string_visitor{}, m);
    }
  } // namespace model_type

  namespace algo
  {
    std::string algo_string_visitor::operator()(const PDR& a) const
    {
      return format("Running PDR algorithm.\n For {}.", to_string(a.model));
    }

    std::string algo_string_visitor::operator()(const IPDR& a) const
    {
      return format("Running IPDR algorithm.\n For {}.", to_string(a.model));
    }

    std::string algo_string_visitor::operator()(const Bounded& a) const
    {
      return format("Running Bounded algorithm.\n For {}.", to_string(a.model));
    }

    std::string to_string(const std::variant<PDR, IPDR, Bounded>& a)
    {
      return std::visit(algo_string_visitor{}, a);
    }
  } // namespace algo

  // ARGUMENTLIST PUBLIC
  //

  ArgumentList::ArgumentList(int argc, char* argv[])
  {
    cxxopts::Options clopt = make_options(argv[0]);
    try
    {
      cxxopts::ParseResult clresult = clopt.parse(argc, argv);

      if (clresult.count("help"))
      {
        std::cerr << clopt.help() << std::endl;
        exit(0);
      }

      parse_verbosity(clresult);
      parse_model(clresult);
      if (onlyshow)
        return;
      parse_tactic(clresult);
    }
    catch (const std::exception& e)
    {
      std::cerr << "Error parsing command line arguments" << std::endl
                << std::endl;
      std::cerr << clopt.help() << std::endl;
      throw;
    }
  }

  void ArgumentList::show_header(std::ostream& out) const
  {
    using std::endl;
    switch (tactic)
    {
      case pdr::Tactic::basic:
        out << format("Finding {}-pebble strategy for {}",
            starting_value.value(), model_name);
        break;
      case pdr::Tactic::decrement:
        out << format("Finding minimal pebble strategy for {} by decrementing",
            model_name);
        break;
      case pdr::Tactic::increment:
        out << format("Finding minimal pebble strategy for {} by incrementing",
            model_name);
        break;
      case pdr::Tactic::inc_jump_test:
        out << format("{} and +10 step jump test for {} by incrementing",
            starting_value.value(), model_name);
        break;
      case pdr::Tactic::inc_one_test:
        out << format("{} and +1 step jump test for {} by incrementing",
            starting_value.value(), model_name);
        break;
      default: throw std::invalid_argument("pdr::Tactic is undefined");
    }
    out << endl;
    if (exp_sample)
      out << format("Running an experiment with {} samples. ", *exp_sample);
    if (exp_control)
      out << "(a control run)";
    out << endl;

    if (delta)
      out << "Using delta-encoded frames." << endl;
    if (rand)
      out << "Using randomized seed." << endl;
    if (seed)
      out << "Using seed: " << *seed << endl;
    if (tseytin)
      out << "Using tseytin encoded transition." << endl;
    out << endl;
  }

  const FolderStructure ArgumentList::make_folders() const
  {
    FolderStructure F;
    F.run_type_dir = base_out() / (exp_sample ? "experiments" : "runs");

    F.model_type_dir = F.run_type_dir;
    {
      if (bounded)
        F.model_type_dir /= "bounded";
      else if (peter)
        F.model_type_dir /= "peter";
      else
        F.model_type_dir /= "pebbling";
    }

#warning todo: formal mechanism for peterston name
    F.model_dir = F.model_type_dir / model_name;
    F.run_dir   = F.model_dir / run_folder_name();
    F.analysis  = F.run_dir / "analysis";
    F.file_base = file_name();

    fs::create_directories(F.analysis);

    return F;
  }

  string ArgumentList::file_name() const
  {
    string file_string = format("{}-{}", model_name, to_string(tactic));

    if (!exp_sample && starting_value)
      file_string += format("-{}", starting_value.value());

    if (delta)
      file_string += "-delta";

    return file_string;
  }

  string ArgumentList::run_folder_name() const
  {
    string folder_string = to_string(tactic);

    if (exp_sample)
      folder_string += format("-exp{}", *exp_sample);
    else if (starting_value)
      folder_string += format("-{}", starting_value.value());

    if (delta)
      folder_string += "-delta";

    return folder_string;
  }

  fs::path ArgumentList::create_model_dir() const
  {
    fs::path results_folder = base_out();
    if (exp_sample)
      results_folder /= "experiments";

    if (bounded)
      results_folder /= "bounded";
    else if (peter)
      results_folder /= "peter";
    else
      results_folder /= "pebbling";

    return setup(results_folder / model_name);
  }

  fs::path ArgumentList::setup_run_path() const
  {
    return setup(base_out() / "results" / model_name / run_folder_name());
  }

  // PRIVATE MEMBERS
  // Constructor helpers

  namespace
  {
    // cxx string for option with shorthand
    string sh(char abb, string name) { return fmt::format("{},{}", abb, name); }
  } // namespace

  cxxopts::Options ArgumentList::make_options(std::string name)
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
      (sh('v', s_verbose), "Output all messages during pdr iterations")
      (sh('w', s_whisper), "Output only some messages during pdr iterations. (default)")
      (sh('s', s_silent), "Output no messages during pdr iterations.")
      ("out-file", "Write to an output file instead of standard output", 
        value< optional<string> >(out), "(string:FILE.out)")

      (s_pdr, "Run a single iteration of pdr for the given model.")
      (s_ipdr, "Run an iteration ipdr, consisting of multiple pdr iterations.")
      (s_bounded, "Run the bounded model checking approach for pebbling.")

      (s_pebbling, "Use the reversible pebble game and find a minimal strategy.")
      (s_peter, "Use the peterson protocol for mutual exclusion with at most N processes",
       value< optional<unsigned> >(), "(uint: N)")

      // tactic option
      (sh('e', s_exp), "run an experiment with I iterations.",
        value< optional<unsigned> >(exp_sample), "(uint: I)")
      (s_control, "Run only a control experiment (no ipdr).",
        value<bool>(exp_control))

      (s_pebbles, "Number of pebbles for a single pebbling pdr run.",
       value< optional<unsigned> >(starting_value), "(uint)")
      (s_procs, "Number of processes for a single peterson pdr run, or the starting value for ipdr.",
       value< optional<unsigned> >(starting_value), "(uint)")

      // model options
      (s_dir,"Directory (relative to ./) than contains runable benchmarks.",
        value<fs::path>()->default_value(my::io::BENCH_FOLDER), "(string:F)")
      (s_bench, "File in in .bench format.",
        value<string>(), "(string:FILE)")
      (s_tfc, "File in in .tfc format.",
        value<string>(), "(string:FILE)")
      (s_hop, "Construct h-operator model from provided bitwidth (BITS) and modulus (MOD).",
        value< vector<unsigned> >(), "(uint:BITS,uint:MOD)")

      // context options
      (sh('r', s_rand), "Use a randomized seed for the SAT solver with time(0)",
        value<bool>(rand))
      (s_seed, "Use the given seed for the SAT solver",
        value< optional<unsigned> >(seed), "(uint:SEED)")
      (s_tseytin, "Build the transition relation using the tseytin reform.",
        value<bool>(tseytin))
      (s_show, "Only write the given model to its output file, does not run the algorithm.",
       value<bool>(onlyshow))

      (s_mic, "The number of times N that pdr retries dropping a literal.")

      ("h,help", "Show usage");
    // clang-format on
#warning add mic_retries option
    return clopt;
  }

  void ArgumentList::parse_verbosity(const cxxopts::ParseResult& clresult)
  {
    if (clresult.count("verbose"))
      verbosity = OutLvl::verbose;
    else if (clresult.count("whisper"))
      verbosity = OutLvl::whisper;
    else if (clresult.count("silent"))
      verbosity = OutLvl::silent;
    else
      verbosity = OutLvl::whisper;
  }

  namespace
  {
    bool one_of(
        std::initializer_list<string> names, const cxxopts::ParseResult& r)
    {
      unsigned occurences = std::accumulate(names.begin(), names.end(), 0,
          [&r](unsigned a, const string& name) { return a + r.count(name); });

      return occurences == 1;
    }
  } // namespace

  void ArgumentList::parse_tactic(const cxxopts::ParseResult& clresult)
  {
    assert(one_of({ "pdr", "ipdr", "bounded" }, clresult));
    assert(one_of({ "pebbling", "peter" }, clresult));

    if (clresult.count("peter"))
    {
      model_type::Peterson peter;
      if (clresult.count("procs"))
        peter.start = clresult["procs"].as<unsigned>();
      else
        throw std::invalid_argument(
            "peterson requires a number of (starting) processes");

      if (clresult.count("max_procs"))
    }
    if (clresult.count("pdr")) {}

    if (clresult.count("optimize"))
    {
      std::string tactic_str = clresult["optimize"].as<std::string>();
      if (tactic_str == "inc")
        tactic = pdr::Tactic::increment;
      else if (tactic_str == "dec")
        tactic = pdr::Tactic::decrement;
      else
        throw std::invalid_argument(format(
            "optimize must be either {} or {}.", increment_str, decrement_str));
    }
    else if (clresult.count(inc_jump_str))
    {
      tactic         = pdr::Tactic::inc_jump_test;
      starting_value = clresult[inc_jump_str].as<unsigned>();
    }
    else if (clresult.count(inc_one_str))
    {
      tactic         = pdr::Tactic::inc_one_test;
      starting_value = clresult[inc_one_str].as<unsigned>();
    }
    else
    {
      tactic = pdr::Tactic::basic;
      if (!clresult.count("bounded"))
        if (!clresult.count("pebbles"))
          throw std::invalid_argument(
              "Basic run requires a \"pebbles\" value.");

      // max_pebbles = clresult["pebbles"].as<unsigned>();
      // if (max_pebbles < 0)
      //   throw std::invalid_argument("pebbles must be positive.");
    }

    if (clresult.count("peter"))
      tactic = pdr::Tactic::increment;
    else if (clresult.count("bounded"))
      tactic = pdr::Tactic::decrement;

    unsigned n_tests =
        clresult.count(pdr::tactic::to_string(pdr::Tactic::inc_jump_test)) +
        clresult.count(pdr::tactic::to_string(pdr::Tactic::inc_one_test));
    if (n_tests > 1)
      throw std::invalid_argument("specify at most one test");
  }

  void ArgumentList::parse_model(const cxxopts::ParseResult& clresult)
  {
    unsigned n_models = clresult.count("hop") + clresult.count("tfc") +
                        clresult.count("bench") + clresult.count("peter");
    if (n_models != 1)
      throw std::invalid_argument(
          "specify one of tfc, bench, or hop. or peter.");

    if (clresult.count("hop"))
    {
      model = ModelType::hoperator;

      const auto hop_list = clresult["hop"].as<std::vector<unsigned>>();
      if (hop_list.size() != 2)
        throw std::invalid_argument(
            "hop requires two positive integers: bitwidth,modulus");
      hop        = { hop_list[0], hop_list[1] };
      model_name = format("hop{}_{}", hop_list[0], hop_list[1]);
    }
    if (clresult.count("tfc"))
    {
      model      = ModelType::tfc;
      model_name = clresult["tfc"].as<std::string>();
    }
    if (clresult.count("bench"))
    {
      model      = ModelType::bench;
      model_name = clresult["bench"].as<std::string>();
    }

    bench_folder = my::io::BENCH_FOLDER / clresult["dir"].as<fs::path>();
  }

} // namespace my::cli
