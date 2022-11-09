#include "cli-parse.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "parse_tfc.h"
#include "tactic.h"
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <initializer_list>
#include <lorina/bench.hpp>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <variant>

#warning display settings for: delta, seed, mic_retries
namespace my::cli
{
  namespace fs = ghc::filesystem;
  using namespace pdr::tactic;
  using fmt::format;
  using std::string;

  // STRUCTS
  //
  namespace graph_src
  {
    // NAME
    struct src_name_visitor
    {
      std::string operator()(const benchFile& a) const { return a.name; }

      std::string operator()(const tfcFile& a) const { return a.name; }

      std::string operator()(const Hop& a) const
      {
        return format("Hop{}_{}", a.bits, a.mod);
      }
    };

    std::string get_name(const Graph_var& g)
    {
      return std::visit(src_name_visitor{}, g);
    }

    // GRAPH
    struct src_graph_visitor
    {
      dag::Graph operator()(const benchFile& a) const
      {

        mockturtle::klut_network klut;
        auto const result =
            lorina::read_bench(a.file.string(), mockturtle::bench_reader(klut));
        if (result != lorina::return_code::success)
          throw std::invalid_argument(
              a.file.string() + " is not a valid .bench file");

        return dag::from_dot(klut, a.name);
      }

      dag::Graph operator()(const tfcFile& a) const
      {
        parse::TFCParser parser;
        return parser.parse_file(a.file.string(), a.name);
      }

      dag::Graph operator()(const Hop& a) const
      {
        return dag::hoperator(a.bits, a.mod);
      }
    };

    dag::Graph get_graph(const Graph_var& g)
    {
      return std::visit(src_graph_visitor{}, g);
    }
  } // namespace graph_src

  namespace model_type
  {
    // STRING
    struct model_t_string_visitor
    {
      std::string operator()(const t_Pebbling& m) const
      {
        if (m.max_pebbles)
          return format("Pebbling algorithm. {} pebbles.", *m.max_pebbles);
        else
          return "Pebbling algorithm.";
      }

      std::string operator()(const t_Peterson& m) const
      {
        return format(
            "Peterson algorithm. {} processes out of {} max.", m.start, m.max);
      }
    };

    std::string to_string(const Model_var& m)
    {
      return std::visit(model_t_string_visitor{}, m);
    }

    // TAG
    struct model_t_tag_visitor
    {
      std::string operator()(const t_Pebbling& m) const
      {
        if (m.max_pebbles)
          return format("-pebbling_{}", *m.max_pebbles);
        else
          return "-pebbling";
      }

      std::string operator()(const t_Peterson& m) const
      {
        return format("-peter_{}_{}", m.start, m.max);
      }
    };

    std::string filetag(const Model_var& m)
    {
      return std::visit(model_t_tag_visitor{}, m);
    }
  } // namespace model_type

  namespace algo
  {
    // TOSTRING
    struct algo_string_visitor
    {
      std::string operator()(const PDR& a) const
      {
        return format("Running PDR algorithm.\n For {}.", to_string(a.model));
      }

      std::string operator()(const IPDR& a) const
      {
        return format("Running IPDR algorithm.\n For {}.", to_string(a.model));
      }

      std::string operator()(const Bounded& a) const
      {
        return format(
            "Running Bounded algorithm.\n For {}.", to_string(a.model));
      }
    };

    std::string to_string(const Algo_var& a)
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
      parse_alg(clresult);

      if (onlyshow)
        return;

      parse_run(clresult);
    }
    catch (const std::exception& e)
    {
      std::cerr << "Error parsing command line arguments" << std::endl
                << std::endl;
      std::cerr << clopt.help() << std::endl;
      throw;
    }

    folders.run_type_dir = base_out() / (experiment ? "experiments" : "runs");

    folders.model_type_dir = folders.run_type_dir;
    {
      if (bounded)
        folders.model_type_dir /= "bounded";
      else if (peter)
        folders.model_type_dir /= "peter";
      else
        folders.model_type_dir /= "pebbling";
    }

#warning todo: formal mechanism for peterston name
    folders.model_dir = folders.model_type_dir / model_name;
    folders.run_dir   = folders.model_dir / run_folder_name();
    folders.analysis  = folders.run_dir / "analysis";
    folders.file_base = file_name();

    fs::create_directories(folders.analysis);
  }

  void ArgumentList::show_header(std::ostream& out) const
  {
    using std::endl;
    out << algo::to_string(algorithm) << endl;

    if (experiment)
    {
      out << format(
          "Running an experiment with {} samples. ", experiment->repetitions);
      if (experiment->control)
        out << "(a control run)";
    }
    out << endl;

    if (rand)
      out << "Using randomized seed." << endl;
    if (seed)
      out << "Using seed: " << *seed << endl;
    if (tseytin)
      out << "Using tseytin encoded transition." << endl;
    out << endl;
  }

  string ArgumentList::file_name() const
  {
    string file_string = format("{}-{}", model_name, to_string(tactic));

    file_string += model_type::to_string(algo::model(algorithm));

    return file_string;
  }

  string ArgumentList::run_folder_name() const
  {
    string folder_string = to_string(tactic);

    if (experiment)
      folder_string += format("-exp{}", experiment->repetitions);
    else
      folder_string += model_type::to_string(algo::model(algorithm));

    return folder_string;
  }

  fs::path ArgumentList::create_model_dir() const
  {
    fs::path results_folder = base_out();
    if (experiment)
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
    string sh(char shorthand, string name)
    {
      return format("{},{}", shorthand, name);
    }
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

      (sh('a', o_alg), 
       format("Run the \"{}\", \"{}\", or \"{}\" model checking algorithm.", s_pdr, s_ipdr, s_bounded),
       value<string>()->default_value(s_pdr))

      (s_pebbling, "Use the reversible pebble game as a transition system and find a minimal strategy.")
      (s_peter, "Use the peterson protocol for mutual exclusion with at most N processes as a transition system.",
       value< unsigned >(), "(uint: N)")

      // tactic option
      (sh('e', s_exp), "run an experiment with I iterations.",
        value< unsigned >(), "(uint: I)")
      (sh('c', s_control), "Run only a control experiment (no ipdr).",
        value<bool>())
      (sh('i', o_inc), format("Specify the constraining ({}) or relaxing ({}) version of ipdr."
          "Automatically selected for a transition system if empty.", s_constrain, s_relax))

      (s_pebbles, "Number of pebbles for a single pebbling pdr run.",
       value< optional<unsigned> >(), "(uint)")
      (s_procs, "Number of processes for a single peterson pdr run, or the starting value for ipdr.",
       value< optional<unsigned> >(), "(uint)")

      // model options
      (s_dir,"Directory (relative to ./) than contains runable benchmarks.",
        value<fs::path>(folders.bench_src)->default_value(my::io::BENCH_FOLDER), "(string:F)")
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

      // (s_mic, "The number of times N that pdr retries dropping a literal.")

      ("h,help", "Show usage");
    // clang-format on
#warning add mic_retries option
    return clopt;
  }

  void ArgumentList::parse_verbosity(const cxxopts::ParseResult& clresult)
  {
    if (clresult.count(s_verbose))
      verbosity = OutLvl::verbose;
    else if (clresult.count(s_whisper))
      verbosity = OutLvl::whisper;
    else if (clresult.count(s_silent))
      verbosity = OutLvl::silent;
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

  void ArgumentList::parse_alg(const cxxopts::ParseResult& clresult)
  {
    assert(one_of({ s_pdr, s_ipdr, s_bounded }, clresult));
    assert(one_of({ s_pebbling, s_peter }, clresult));

    if (clresult.count(s_peter))
    {
      model_type::t_Peterson peter;

      if (clresult.count(s_procs))
        peter.start = clresult[s_procs].as<unsigned>();
      else
        throw std::invalid_argument(
            "peterson requires a number of (starting) processes");

      peter.max = clresult[s_peter].as<unsigned>();
      model     = peter;
    }
    else if (clresult.count(s_pebbling))
    {
      model_type::t_Pebbling pebbling;
      pebbling.model = parse_graph_src(clresult);
      if (clresult.count(s_pebbles))
        pebbling.max_pebbles = clresult[s_pebbles].as<unsigned>();

      model = pebbling;
    }

    if (clresult.count(s_pdr))
    {
      algorithm = algo::PDR();
    }
    else if (clresult.count(s_ipdr))
    {
      pdr::Tactic t;
      if (clresult.count(o_inc))
      {
        std::string tactic_str = clresult[o_inc].as<std::string>();

        if (tactic_str == s_relax)
          t = pdr::Tactic::relax;
        else if (tactic_str == s_constrain)
          t = pdr::Tactic::constrain;
        else
          throw std::invalid_argument(format(
              "--{} must be either {} or {}.", o_inc, s_constrain, s_relax));
      }

      if (clresult.count(s_pebbling))
        t = pdr::Tactic::constrain;
      else if (clresult.count(s_peter))
        t = pdr::Tactic::relax;
      else
        assert(false);

      algorithm = alg::IPDR(t);
    }
    else if (clresult.count(s_bounded))
    {
      algorithm = algo::Bounded(m);
    }
  }

  void ArgumentList::parse_run(const cxxopts::ParseResult& clresult)
  {
    if (clresult.count(o_inc))
    {
      if (clresult.count(s_pdr) || clresult.count(s_bounded))
        std::cerr << format("WARNING: {} is unused in {} and {}", o_inc, s_pdr,
                         s_bounded)
                  << std::endl;
    }
    else if (clresult.count(s_bounded))
    {
      if (!clresult.count(s_pebbling))
        throw std::invalid_argument(
            "Bounded model checking is supported for Pebbling only.");
    }

    if (clresult.count(s_exp))
    {
      unsigned reps{ clresult[s_exp].as<unsigned>() };
      if (clresult.count(s_pdr))
        throw std::invalid_argument("Experiments verify incremental runs only");

      bool control = (clresult.count(s_control) == 1);
      experiment   = { reps, control };
    }
  }

  graph_src::Graph_var ArgumentList::parse_graph_src(
      const cxxopts::ParseResult& clresult)
  {
    if (!one_of({ s_bench, s_tfc, s_hop }, clresult))
      throw std::invalid_argument("Pebbling requires (only) one of a .bench or "
                                  ".tfc inputfile, or a Hop specification.");

    graph_src::Graph_var rv;

    if (clresult.count(s_hop))
    {
      const std::vector<unsigned> hop_list =
          clresult["hop"].as<std::vector<unsigned>>();

      if (hop_list.size() != 2)
        throw std::invalid_argument(format(
            "{} requires two positive integers: bitwidth,modulus", s_hop));

      rv         = graph_src::Hop{ hop_list[0], hop_list[1] };
      model_name = graph_src::name(rv);
    }
    else if (clresult.count(s_tfc))
    {
      model_name = clresult[s_tfc].as<std::string>();
      rv         = graph_src::tfcFile{ model_name,
        io::file_in(folders.bench_src, model_name, ".tfc") };
    }
    else if (clresult.count(s_bench))
    {
      model_name = clresult[s_bench].as<std::string>();
      rv         = graph_src::benchFile{ model_name,
        io::file_in(folders.bench_src, model_name, ".bench") };
    }
    else
    {
      assert(false);
    }

    // bench_folder = my::io::BENCH_FOLDER / clresult[s_dir].as<fs::path>();

    return rv;
  }

} // namespace my::cli
