#include "cli-parse.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "parse_tfc.h"
#include "tactic.h"
#include "types-ext.h"

#include <cassert>
#include <cxxopts.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <initializer_list>
#include <lorina/bench.hpp>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <variant>
#include <vector>

#warning display settings for: delta, seed, mic_retries
namespace my::cli
{
  using namespace pdr::tactic;
  using fmt::format;
  using std::optional;
  using std::string;
  using std::string_view;
  using std::vector;

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
        assert(a.type == pdr::Tactic::constrain ||
               a.type == pdr::Tactic::relax ||
               a.type == pdr::Tactic::binary_search);
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
    cxxopts::Options clopt        = make_options(argv[0]);
    // try
    // {
    cxxopts::ParseResult clresult = clopt.parse(argc, argv);

    if (clresult.count("help"))
    {
      clopt.show_positional_help();
      std::cerr << clopt.help() << std::endl;
      exit(0);
    }

    parse_problem(clresult);
    parse_alg(clresult);
    parse_mode(clresult);
    parse_verbosity(clresult);
    parse_context(clresult);

    // if (!onlyshow)
    //   parse_mode(clresult);
    // }
    // catch (std::exception const& e)
    // {
    //   std::cerr << e.what() << std::endl
    //             << "Error parsing command line arguments" << std::endl
    //             << std::endl
    //             << clopt.help() << std::endl;
    //   throw;
    // }

    folders.run_type_dir = base_out() / (experiment ? "experiments" : "runs") /
                           algo::get_name(algorithm);

    if (z3pdr)
      folders.model_type_dir = folders.run_type_dir / "z3pdr";
    else
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
      std::vector<std::string> tags;
      tags.push_back(model_t::filetag(model));
      tags.push_back(algo::filetag(algorithm));
      if (experiment)
        tags.push_back(format("exp_{}", experiment->repetitions));
      if (control_run)
        tags.push_back("C");
      for (std::string_view t : tags)
        folders.file_base += format("-{}", t);
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

    if (control_run)
      out << "a control run (no ipdr optimization)." << std::endl;

    if (experiment)
    {
      out << format(
          "Running an experiment with {} samples. ", experiment->repetitions);
      if (control_run)
        out << "(a control run)";
      out << endl;
    }

    if (auto rand = variant::get_cref<bool>(r_seed))
    {
      if (rand.value())
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
    clopt.positional_help(format("{} {} {}", o_problem, o_alg, o_mode));
    clopt.add_options("positional parameter")
      (o_problem, 
       format("Solve the Reversible Pebbling Problem "
         "or verify correctness of the Peterson Protocol:\n{}", problem_group),
       value<string>())
      (o_alg, 
       format("Choose an algorithm to use:\n{}.", algo_group),
       value<string>())
      (o_mode, format("Run a single run or perform an experiment of multiple runs:\n{}", mode_group),
       value<string>());

    clopt.add_options()
      ("h,help", "Show usage")

      // 
      (s_z3pdr, "Use Z3's fixedpoint engine for pdr (spacer)",
       value<bool>(z3pdr)->default_value("false"))
      (sh('c', s_control), 
        "Run only a naive ipdr version (no incremental optimization). Or perform only naive runs in an experiment.",
        value<bool>(control_run)->default_value("false"))

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
      (sh('r', s_rand), "Use a randomized seed for the SAT solver with time(0)")
      (s_seed, "Use the given seed for the SAT solver",
        value<unsigned>(), "(uint:SEED)")
      (s_tseytin, "Build the transition relation using z3's tseytin reform.",
        value<bool>(tseytin)->default_value("false"))
      (s_show, "Only write the given model to its output file, does not run the algorithm.",
        value<bool>(onlyshow)->default_value("false"))

      (s_mic, "Limit on the number of times N that pdr retries dropping a literal in MIC. Unlimited by default.",
       value<unsigned>(), "(uint:N)");

    clopt.add_options("output-level")
      (sh('v', s_verbose), "Output all messages during pdr iterations")
      (sh('w', s_whisper), "Output only some messages during pdr iterations. (default)")
      (sh('s', s_silent), "Output no messages during pdr iterations.")
      ("out-file", "Write to an output file instead of standard output", 
        value< optional<string> >(out), "(string:FILE.out)");

    //  problems
    clopt.add_options(s_pebbling)
      (s_pebbles, "Number of pebbles for a single pebbling pdr run.",
       value<unsigned>(), "(uint)");

    clopt.add_options(s_peter)
      (s_mprocs, "REQUIRED. The maximum number of processes for the Peterson Protocol transition system",
       value<unsigned>(), "(uint)")
      (s_procs, "REQUIRED. Number of processes for a single peterson pdr run, or the starting value for ipdr.",
       value<unsigned>(), "(uint)");

    // algorithms
    clopt.add_options(s_ipdr)
      (sh('i', o_inc), 
       format("Specify the constraining (\"{}\"), relaxing (\"{}\") or binary search (\"{}\") version of ipdr."
          "Automatically selected for a transition system if empty.", s_constrain, s_relax, s_binary), 
       value<string>());

    // modes
    clopt.add_options(s_run);

    clopt.add_options(s_exp)
      (s_its, "run an experiment with I iterations.",
        value< unsigned >(), "(uint: I)")
      (s_seeds, "A list of seeds to be used by an experiment",
       value<vector<unsigned>>(), "(uint,uint,...)");
    // clang-format on

    clopt.parse_positional({ o_problem, o_alg, o_mode });

    return clopt;
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
        throw std::invalid_argument(
            format("At most one of `{}` allowed. Have {}", names, n));
    }

    void require_one_of(
        std::initializer_list<string> names, cxxopts::ParseResult const& r)
    {
      unsigned n = occurences(names, r);
      if (n != 1)
        throw std::invalid_argument(
            format("One of `{}` required. Have {}.", names, n));
    }

    void is_one_of(string_view name, vector<string> const& names)
    {
      if (std::find(names.cbegin(), names.cend(), name) == names.end())
        throw std::invalid_argument(
            format("`{}` must be one of `{}`", name, names));
    }

    void ignored(
        std::initializer_list<string> names, cxxopts::ParseResult const& r)
    {
      for (string const& name : names)
      {
        if (r.count(name))
          std::cerr << format("WARNING: argument `{}` is ignored "
                              "for this execution",
                           name)
                    << std::endl;
      }
    }
  } // namespace

  void ArgumentList::parse_problem(cxxopts::ParseResult const& clresult)
  {
    assert(clresult[o_problem].count());
    std::string problem = clresult[o_problem].as<std::string>();
    is_one_of(problem, problem_group);

    if (problem == s_peter)
    {
      ignored({ s_pebbles }, clresult);
      require_one_of({ s_mprocs }, clresult);
      require_one_of({ s_procs }, clresult);

      model_t::Peterson peter;

      peter.max   = clresult[s_mprocs].as<unsigned>();
      peter.start = clresult[s_procs].as<unsigned>();

      model = peter;
    }
    else
    {
      assert(problem == s_pebbling);
      ignored({ s_mprocs, s_procs }, clresult);

      model_t::Pebbling pebbling;
      pebbling.src = parse_graph_src(clresult);

      if (clresult.count(s_pebbles))
        pebbling.max_pebbles = clresult[s_pebbles].as<unsigned>();

      model = pebbling;
    }
  }

  void ArgumentList::parse_alg(cxxopts::ParseResult const& clresult)
  {
    assert(clresult[o_alg].count());
    string algo = clresult[o_alg].as<string>();
    is_one_of(algo, algo_group);

    if (clresult[s_mic].count())
      mic_retries = clresult[s_mic].as<unsigned>();

    if (algo == s_pdr)
    {
      ignored({ o_inc }, clresult);
      algorithm = algo::t_PDR();
    }
    else if (algo == s_ipdr)
    {
      pdr::Tactic t;

      // default
      if (is<model_t::Pebbling>(model))
        t = pdr::Tactic::constrain;
      else
      {
        assert(is<model_t::Peterson>(model));
        t = pdr::Tactic::relax;
      }

      // override default
      if (clresult.count(o_inc))
      {
        string tactic_str = clresult[o_inc].as<string>();
        t                 = pdr::tactic::mk_tactic(tactic_str);
      }

      algorithm = algo::t_IPDR(t);
    }
    else
    {
      assert(algo == s_bounded);
      ignored({ o_inc }, clresult);

      if (!is<model_t::Pebbling>(model))
        throw std::invalid_argument(
            "Bounded model checking is supported for Pebbling only.");

      algorithm = algo::t_Bounded();
    }

    // z3pdr is automatically set
    // the z3 fixedpoint implementation does only naive (control) runs
    control_run = z3pdr ? true : control_run;
    if (z3pdr && control_run)
    {
      std::cerr
          << fmt::format(
                 "WARNING: z3pdr is only used as a reference and performs no "
                 "incremental optimization (`{}` defaults to true)",
                 s_control)
          << std::endl;
    }
  }

  void ArgumentList::parse_mode(cxxopts::ParseResult const& clresult)
  {
    string mode = clresult[o_mode].as<string>();
    is_one_of(mode, mode_group);

    if (mode == s_run)
    {
      ignored({ s_its, s_seeds }, clresult);
      if (is<algo::t_PDR>(algorithm) && !experiment)
        throw std::invalid_argument(format(
            "pebbling pdr run requires a starting number of pebbles: `{}`",
            s_pebbles));
    }
    else
    {
      assert(mode == s_exp);
      require_one_of({ s_its }, clresult);
      ignored({ s_procs, s_pebbles }, clresult);

      unsigned reps = clresult[s_its].as<unsigned>();
      optional<vector<unsigned>> seeds;

      if (clresult.count(s_seeds))
        seeds = clresult[s_seeds].as<vector<unsigned>>();

      if (clresult.count(s_pdr))
        throw std::invalid_argument(
            "Experiments verify incremental (non-pdr) runs only");

      experiment = { reps, seeds };
    }
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

  void ArgumentList::parse_context(cxxopts::ParseResult const& clresult)
  {
    atmost_one_of({ s_rand, s_seed }, clresult);

    if (clresult.count(s_rand))
      r_seed = true;
    else if (clresult.count(s_seed))
      r_seed = clresult[s_seed].as<unsigned>();

    if (clresult.count(s_mic))
      mic_retries = clresult[s_mic].as<unsigned>();

    // s_tseytin and s_show are set automatically
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
    else
    {
      assert(clresult.count(s_bench));

      string name = clresult[s_bench].as<string>();
      name        = strip_extension(name, "bench");
      rv = graph_src::benchFile{ name, folders.src_file(name, "bench") };
    }

    return rv;
  }

} // namespace my::cli
