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
    ModelType model;
    std::string model_name;
    hop_arg hop;
    fs::path bench_folder;
    std::optional<std::string> out;

    std::optional<unsigned> max_pebbles;

    // run options
    bool rand;
    std::optional<unsigned> seed;
    bool delta;
    bool tseytin;
    bool onlyshow;
    pdr::Tactic tactic;

    bool _failed = false;
  };


  std::string to_string(pdr::Tactic r);
  void show_header(const ArgumentList& clargs);

  Options make_options(std::string name, ArgumentList& clargs);
  void parse_verbosity(ArgumentList& clargs, const ParseResult& clresult);
  void parse_tactic(ArgumentList& clargs, const ParseResult& clresult);
  void parse_model(ArgumentList& clargs, const ParseResult& clresult);
  ArgumentList parse_cl(int argc, char* argv[]);
} // namespace my::cli
#endif // CLI_H
