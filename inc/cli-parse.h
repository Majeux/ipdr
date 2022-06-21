#ifndef CLI_H
#define CLI_H

#include "logger.h"
#include "tactic.h"
#include <cxxopts.hpp>
#include <string>

namespace my::cli
{
  using namespace cxxopts;
  namespace fs = ghc::filesystem;

  struct hop_arg
  {
    unsigned bits;
    unsigned mod;
  };

  enum class ModelType
  {
    none,
    tfc,
    bench,
    hoperator
  };

  struct ArgumentList
  {
    OutLvl verbosity;
    // the way pdr is run
    pdr::Tactic tactic = pdr::Tactic::undef;
    // if given: run an experiment in with this many samples
    std::optional<unsigned> exp_sample;
    // comparison run for optimizer experiments
    bool experiment_control;
    ModelType model;
    // folder that contains the model_name file
    fs::path bench_folder;
    // file_name (without extension) for the model
    std::string model_name;
    // H-operator model, instead of reading a graph
    hop_arg hop;
    std::optional<std::string> out; // filename to redirect logging out messages

    std::optional<unsigned> max_pebbles; // the constraint on no. marked n_nodes

    // run options
    bool rand; // use a seed for the solver. generated by time(0)
    std::optional<unsigned> seed; // the seed for the solver
    bool delta = false; // run pdr using a delta encoding for the F-sequence
    bool tseytin; // encode pebbling::Model transition using tseyting enconding

    bool onlyshow; // only read in and produce the model image and description

    bool bounded;

    bool _failed = false;
  };

  void show_header(const ArgumentList& clargs);

  Options make_options(std::string name, ArgumentList& clargs);
  void parse_verbosity(ArgumentList& clargs, const ParseResult& clresult);
  void parse_tactic(ArgumentList& clargs, const ParseResult& clresult);
  void parse_model(ArgumentList& clargs, const ParseResult& clresult);
  ArgumentList parse_cl(int argc, char* argv[]);
} // namespace my::cli
#endif // CLI_H