#ifndef CLI_H
#define CLI_H

#include "dag.h"
#include "io.h"
#include "logger.h"
#include "tactic.h"

#include <array>
#include <cxxopts.hpp>
#include <fmt/core.h>
// #include <filesystem>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace my::cli
{
  using namespace cxxopts;
  using namespace my::io;

  class ArgumentList;

  namespace graph_src
  {
    struct benchFile
    {
      std::string name;
      fs::path file;
    };

    struct tfcFile
    {
      std::string name;
      fs::path file;
    };

    struct Hop
    {
      unsigned bits;
      unsigned mod;
      Hop(unsigned b, unsigned m) : bits(b), mod(m) {}
    };

    using Graph_var = std::variant<benchFile, tfcFile, Hop>;

    std::string get_name(Graph_var const& g);
    dag::Graph make_graph(Graph_var const& g);
  } // namespace graph_src

  namespace model_t
  {
    using namespace graph_src;

    struct Pebbling
    {
      std::optional<unsigned> max_pebbles; // starting value for constraint
      Graph_var src;
    };

    struct Peterson
    {
      unsigned processes;
      std::optional<unsigned> switch_bound;
    };

    using Model_var = std::variant<Pebbling, Peterson>;

    std::string src_name(Model_var const& m);
    std::string describe(Model_var const& m);
    std::string get_name(Model_var const& m);
    std::string filetag(Model_var const& m);
  } // namespace model_t

  namespace algo
  {
    using namespace model_t;

    struct t_PDR
    {
    };

    struct t_IPDR
    {
      pdr::Tactic type;
      t_IPDR(pdr::Tactic t) : type(t) {}
    };

    struct t_Bounded
    {
    };

    using Algo_var = std::variant<t_PDR, t_IPDR, t_Bounded>;

    std::string get_name(Algo_var const& a);
    std::string filetag(Algo_var const& a);
    Model_var const& get_model(Algo_var const& a);

  } // namespace algo

  // type checkers
  template <typename T> bool is(graph_src::Graph_var const& a)
  {
    return std::holds_alternative<T>(a);
  }
  template <typename T> bool is(model_t::Model_var const& a)
  {
    return std::holds_alternative<T>(a);
  }
  template <typename T> bool is(algo::Algo_var const& a)
  {
    return std::holds_alternative<T>(a);
  }

  struct Experiment
  {
    unsigned repetitions;
    std::optional<std::vector<unsigned>> seeds;
  };

  class ArgumentList
  {
   public:
    FolderStructure folders;
    OutLvl verbosity{ OutLvl::whisper };
    std::optional<std::string> out; // filename to redirect logging out messages

    algo::Algo_var algorithm;
    model_t::Model_var model;

    std::optional<Experiment> experiment;
    std::variant<bool, unsigned> r_seed;
    std::optional<bool> skip_blocked;
    std::optional<unsigned> mic_retries;
    std::optional<double> subsumed_cutoff;
    std::optional<unsigned> ctg_max_depth;
    std::optional<unsigned> ctg_max_counters;
    bool tseytin;  // encode pebbling::Model transition using tseyting enconding
    bool onlyshow; // only read in and produce the model image and description
    bool control_run;

    bool z3pdr;

    bool _failed = false;

    ArgumentList(int argc, char* argv[]);
    void show_header(std::ostream& out) const;
    const FolderStructure make_folders() const;

   private:
    // constructor helpers
    Options make_options(std::string name);
    void parse_problem(ParseResult const& clresult);
    void parse_alg(ParseResult const& clresult);
    void parse_mode(ParseResult const& clresult);
    void parse_verbosity(ParseResult const& clresult);
    void parse_context(ParseResult const& clresult);
    graph_src::Graph_var parse_graph_src(ParseResult const& clresult);

    // cli names
    inline static const std::string o_alg     = "algo";
    inline static const std::string s_pdr     = "pdr";
    inline static const std::string s_ipdr    = "ipdr";
    inline static const std::string s_bounded = "bounded";
    inline static const std::vector<std::string> algo_group{ s_pdr, s_ipdr,
      s_bounded };

    inline static const std::string o_problem  = "problem";
    inline static const std::string s_pebbling = "pebbling";
    inline static const std::string s_peter    = "peterson";
    inline static const std::vector<std::string> problem_group{ s_pebbling,
      s_peter };

    inline static const std::string s_z3pdr = "z3pdr";

    inline static const std::string o_mode = "mode";
    inline static const std::string s_run  = "run";
    inline static const std::string s_exp  = "experiment";
    inline static const std::vector<std::string> mode_group{ s_run, s_exp };

    inline static const std::string s_its     = "iterations";
    inline static const std::string s_control = "control";
    inline static const std::string s_seeds   = "seeds";

    inline static const std::string o_inc       = "inc";
    inline static const std::string s_constrain = pdr::tactic::constrain_str;
    inline static const std::string s_relax     = pdr::tactic::relax_str;
    inline static const std::string s_binary = pdr::tactic::binary_search_str;

    inline static const std::string s_pebbles = "pebbles";
    inline static const std::string s_mprocs  = "max_procs";
    inline static const std::string s_mswitch = "max_switches";
    inline static const std::string s_procs   = "procs";

    inline static const std::string s_dir   = "dir";
    inline static const std::string s_bench = "bench";
    inline static const std::string s_tfc   = "tfc";
    inline static const std::string s_hop   = "hop";

    inline static const std::string s_rand    = "rand";
    inline static const std::string s_seed    = "seed";
    inline static const std::string s_tseytin = "tseytin";
    inline static const std::string s_show    = "show-only";

    inline static const std::string s_verbose = "verbose";
    inline static const std::string s_whisper = "whisper";
    inline static const std::string s_silent  = "silent";

    inline static const std::string s_skip_blocked = "skip-blocked";
    inline static const std::string s_mic          = "mic-attempts";
    inline static const std::string s_subsumed     = "cut-subsumed";
    inline static const std::string s_ctgdepth     = "ctg-depth";
    inline static const std::string s_ctgnum       = "max-ctgs";
  };
} // namespace my::cli
#endif // CLI_H
