#include "cli-parse.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "parse_tfc.h"
#include "tactic.h"
#include "types-ext.h"

#include <cassert>
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
  using namespace pdr::tactic;
  using fmt::format;
  using std::optional;
  using std::string;

  // STRUCTS
  //
  namespace graph_src
  {
    // NAME
    struct src_name_visitor
    {
      string operator()(benchFile const& a) const { return a.name; }

      string operator()(tfcFile const& a) const { return a.name; }

      string operator()(Hop const& a) const
      {
        return format("Hop{}_{}", a.bits, a.mod);
      }
    };

    string get_name(Graph_var const& g)
    {
      return std::visit(src_name_visitor{}, g);
    }
    //
    // GRAPH
    struct src_graph_visitor
    {
      dag::Graph operator()(benchFile const& a) const
      {

        mockturtle::klut_network klut;
        auto const result =
            lorina::read_bench(a.file.string(), mockturtle::bench_reader(klut));
        if (result != lorina::return_code::success)
          throw std::invalid_argument(
              a.file.string() + " is not a valid .bench file");

        return dag::from_dot(klut, a.name);
      }

      dag::Graph operator()(tfcFile const& a) const
      {
        parse::TFCParser parser;
        return parser.parse_file(a.file.string(), a.name);
      }

      dag::Graph operator()(Hop const& a) const
      {
        return dag::hoperator(a.bits, a.mod);
      }
    };

    dag::Graph make_graph(Graph_var const& g)
    {
      return std::visit(src_graph_visitor{}, g);
    }
  } // namespace graph_src

  //////////////
  namespace model_t
  {
    // STRING
    struct model_t_string_visitor
    {
      string operator()(Pebbling const& m) const
      {
        if (m.max_pebbles)
          return format(
              "pebbling algorithm (set/start {} pebbles)", *m.max_pebbles);
        else
          return "pebbling algorithm.";
      }

      string operator()(Peterson const& m) const
      {
        return format(
            "peterson algorithm. {} processes out of {} max.", m.start, m.max);
      }
    };
    string describe(Model_var const& m)
    {
      return std::visit(model_t_string_visitor{}, m);
    }

    // SRC NAME
    struct model_t_src_visitor
    {
      string operator()(Pebbling const& m) const
      {
        return graph_src::get_name(m.src);
      }

      string operator()(Peterson const& m) const
      {
        return format("peter_{}_{}", m.start, m.max);
      }
    };
    string src_name(Model_var const& m)
    {
      return std::visit(model_t_src_visitor{}, m);
    }

    // FOLDER
    struct model_name_visitor
    {
      string operator()(Pebbling const& m) const
      {
        (void)m;
        return "pebbling";
      }

      string operator()(Peterson const& m) const
      {
        (void)m;
        return "peter";
      }
    };
    string get_name(Model_var const& m)
    {
      return std::visit(model_name_visitor{}, m);
    }

    // TAG
    struct model_t_tag_visitor
    {
      string operator()(Pebbling const& m) const
      {
        if (m.max_pebbles)
          return format("pebbling_{}", *m.max_pebbles);
        else
          return "pebbling";
      }

      string operator()(Peterson const& m) const
      {
        return format("peter_{}_{}", m.start, m.max);
      }
    };

    string filetag(Model_var const& m)
    {
      return std::visit(model_t_tag_visitor{}, m);
    }
  } // namespace model_t

  //////////////
  namespace algo
  {
    // NAME
    struct algo_name_visitor
    {
      string operator()(t_PDR const& a) const
      {
        (void)a;
        return "pdr";
      }

      string operator()(t_IPDR const& a) const
      {
        (void)a;
        return "ipdr";
      }

      string operator()(t_Bounded const& a) const
      {
        (void)a;
        return "bounded";
      }
    };
    string get_name(Algo_var const& m)
    {
      return std::visit(algo_name_visitor{}, m);
    }

    // TAG FOR FILE/FOLDER NAMES
    struct algo_tag_visitor
    {
      string operator()(t_PDR const& a) const
      {
        (void)a;
        return "pdr";
      }

      string operator()(t_IPDR const& a) const
      {
        assert(
            a.type == pdr::Tactic::constrain || a.type == pdr::Tactic::relax);
        return format("ipdr_{}", pdr::tactic::to_string(a.type));
      }

      string operator()(t_Bounded const& a) const
      {
        (void)a;
        return "bounded";
      }
    };
    string filetag(Algo_var const& a)
    {
      return std::visit(algo_tag_visitor{}, a);
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
    catch (std::exception const& e)
    {
      std::cerr << e.what() << std::endl
                << "Error parsing command line arguments" << std::endl
                << std::endl
                << clopt.help() << std::endl;
      throw;
    }

    folders.run_type_dir = base_out() / (experiment ? "experiments" : "runs") /
                           algo::get_name(algorithm);

    folders.model_type_dir = folders.run_type_dir / model_t::get_name(model);

    folders.model_dir = folders.model_type_dir;
    {
      if (auto peb = variant::get_cref<model_t::Pebbling>(model))
        folders.model_dir /= graph_src::get_name(peb->get().src);
      else
      {
        auto peter = variant::get_cref<model_t::Peterson>(model);
        folders.model_dir /=
            format("peter_{}_{}", peter->get().start, peter->get().start);
      }
    }

    folders.file_base = model_t::src_name(model);
    {
      folders.file_base += '-' + model_t::filetag(model);
      folders.file_base += '-' + algo::filetag(algorithm);
      if (experiment)
      {
        if (experiment->control)
          folders.file_base += format("-exp_C_{}", experiment->repetitions);
        else
          folders.file_base += format("-exp_{}", experiment->repetitions);
      }
    }

    folders.run_dir  = folders.model_dir / folders.file_base;
    folders.analysis = folders.run_dir / "analysis";
    fs::create_directories(folders.analysis);
    folders.trace_file.open(folders.file_in_run("trace"));
    folders.solver_dump.open(folders.file_in_run("solver_dump", "solver"));
    folders.model_file = trunc_file(folders.model_dir, "model", "txt");
  }

  void ArgumentList::show_header(std::ostream& out) const
  {
    using std::endl;
    out << format("running {} for {}.", algo::get_name(algorithm),
               model_t::describe(model))
        << std::endl;

    if (experiment)
    {
      out << format(
          "Running an experiment with {} samples. ", experiment->repetitions);
      if (experiment->control)
        out << "(a control run)";
      out << endl;
    }

    if (auto rand = variant::get_cref<bool>(r_seed))
    {
      if (rand)
        out << "Using randomized seed." << endl;
      else
        out << "Using 0-seed." << endl;
    }
    else
      out << format("Using seed: {}.", std::get<unsigned>(r_seed));

    if (tseytin)
      out << "Using tseytin encoded transition." << endl;
    out << endl;
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

  cxxopts::Options ArgumentList::make_options(string name)
  {
    using cxxopts::value;
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
       value<unsigned>(), "(uint)")
      (s_procs, "Number of processes for a single peterson pdr run, or the starting value for ipdr.",
       value<unsigned>(), "(uint)")

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
        value<bool>())
      (s_seed, "Use the given seed for the SAT solver",
        value<unsigned>(), "(uint:SEED)")
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

  void ArgumentList::parse_verbosity(cxxopts::ParseResult const& clresult)
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
    unsigned occurences(
        std::initializer_list<string> names, cxxopts::ParseResult const& r)
    {
      return std::accumulate(names.begin(), names.end(), 0,
          [&r](unsigned a, string const& name) { return a + r.count(name); });
    }

    void atmost_one_of(
        std::initializer_list<string> names, cxxopts::ParseResult const& r)
    {
      unsigned n = occurences(names, r);
      if (n > 1)
        throw std::invalid_argument(format(
            "At most one of `{}` allowed. Have {}", str::ext::join(names), n));
    }

    void require_one_of(
        std::initializer_list<string> names, cxxopts::ParseResult const& r)
    {
      unsigned n = occurences(names, r);
      if (n != 1)
        throw std::invalid_argument(
            format("One of `{}` required. Have {}.", str::ext::join(names), n));
    }
  } // namespace

  void ArgumentList::parse_alg(cxxopts::ParseResult const& clresult)
  {
    require_one_of({ o_alg }, clresult);
    std::string a = clresult[o_alg].as<std::string>();

    if (a == s_pdr)
      algorithm = algo::t_PDR();
    else if (a == s_ipdr)
    {
      pdr::Tactic t;
      if (clresult.count(o_inc))
      {
        string tactic_str = clresult[o_inc].as<string>();

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

      algorithm = algo::t_IPDR(t);
    }
    else if (a == s_bounded)
      algorithm = algo::t_Bounded();
    else
    {
      assert(false);
    }

    require_one_of({ s_pebbling, s_peter }, clresult);
    atmost_one_of({ s_pebbles, s_procs }, clresult);

    if (clresult.count(s_peter))
    {
      model_t::Peterson peter;

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
      model_t::Pebbling pebbling;
      pebbling.src = parse_graph_src(clresult);
      if (clresult.count(s_pebbles))
        pebbling.max_pebbles = clresult[s_pebbles].as<unsigned>();
      else if (a == s_pdr)
        throw std::invalid_argument(
            format("pebbling pdr requires a starting number of pebbles: {}",
                s_pebbles));

      model = pebbling;
    }

    atmost_one_of({ s_rand, s_seed }, clresult);

    if(clresult.count(s_rand))
      r_seed = clresult[s_rand].as<bool>();

    if(clresult.count(s_seed))
      r_seed = clresult[s_seed].as<unsigned>();
  }

  void ArgumentList::parse_run(cxxopts::ParseResult const& clresult)
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

  namespace
  {
    // if filename has an extension, it must be ext.
    // return filename without any extension
    std::string strip_extension(fs::path const& filename, std::string_view ext)
    {
      std::string extension = filename.extension();
      if (extension == format(".{}", ext) || extension == "")
        return filename.stem();

      throw std::invalid_argument(
          format("benchmark file must have extension .{}", ext));
    }
  } // namespace

  graph_src::Graph_var ArgumentList::parse_graph_src(
      cxxopts::ParseResult const& clresult)
  {
    require_one_of({ s_bench, s_tfc, s_hop }, clresult);

    graph_src::Graph_var rv;

    if (clresult.count(s_hop))
    {
      const std::vector<unsigned> hop_list =
          clresult["hop"].as<std::vector<unsigned>>();

      if (hop_list.size() != 2)
        throw std::invalid_argument(format(
            "{} requires two positive integers: bitwidth,modulus", s_hop));

      rv = graph_src::Hop{ hop_list[0], hop_list[1] };
    }
    else if (clresult.count(s_tfc))
    {
      string name = clresult[s_tfc].as<string>();
      name        = strip_extension(name, "tfc");
      rv          = graph_src::tfcFile{ name, folders.src_file(name, "tfc") };
    }
    else if (clresult.count(s_bench))
    {
      string name = clresult[s_bench].as<string>();
      name        = strip_extension(name, "bench");
      rv = graph_src::benchFile{ name, folders.src_file(name, "bench") };
    }
    else
    {
      assert(false);
    }

    // bench_folder = my::io::BENCH_FOLDER / clresult[s_dir].as<fs::path>();

    return rv;
  }

} // namespace my::cli
