#include "bounded.h"
#include "cli-parse.h"
#include "dag.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "mockturtle/networks/klut.hpp"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-experiments.h"
#include "pebbling-model.h"
#include "peterson-experiments.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "types-ext.h"
#include "z3pdr.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cxxopts.hpp>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <iostream>
#include <lorina/bench.hpp>
#include <memory>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <string>
#include <tabulate/table.hpp>
#include <tuple>
#include <variant>
#include <vector>
#include <z3++.h>

using namespace my::cli;
using namespace my::io;

template <typename T> using cc_ptr = T const* const;

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

//
// end OUTPUT

std::ostream& operator<<(std::ostream& o, std::exception const& e)
{
  o << fmt::format(
           "terminated after throwing an \'std::exception\', typeid: {}",
           typeid(e).name())
    << std::endl
    << fmt::format("  what():  {}", e.what()) << std::endl;
  return o;
}

#include <dbg.h>

void handle_pebbling(
    model_t::Pebbling const& descr, ArgumentList& args, pdr::Logger& log)
{
  using namespace pdr::pebbling;
  using my::variant::get_cref;
  using std::endl;

  dag::Graph G = model_t::make_graph(descr.src);
  G.show(args.folders.model_dir / "dag", true);
  log.stats.is_pebbling(G);

  if (args.z3pdr)
  {
    std::cerr << "z3 pdr pebbling test" << std::endl;
    pdr::test::z3PDR z3test(log, G);
    z3test.run();
    std::cerr << "ignoring other arguments" << std::endl;
    return;
  }

  PebblingModel pebbling(args, G);
  pebbling.show(args.folders.model_file);
  pdr::Context context =
      std::holds_alternative<bool>(args.r_seed)
          ? pdr::Context(pebbling, std::get<bool>(args.r_seed))
          : pdr::Context(pebbling, std::get<unsigned>(args.r_seed));

  if (std::holds_alternative<algo::t_PDR>(args.algorithm))
  {
    pebbling.constrain(dbg(descr.max_pebbles).value());
    pdr::PDR pdr_algo(context, pebbling, log);

    pdr::PdrResult r = pdr_algo.run();
    {
      std::cout << "result" << std::endl;
      tabulate::Table T;
      {
        T.add_row(
            { pdr::PdrResult::fields.cbegin(), pdr::PdrResult::fields.cend() });
        auto const row2 = r.listing();
        T.add_row({ row2.cbegin(), row2.cend() });
      }
      std::cout << T << endl << endl;
      std::cout << pdr::result::trace_table(r, pebbling);
      args.folders.trace_file << T << endl << endl;
      args.folders.trace_file << pdr::result::trace_table(r, pebbling);
      pdr_algo.show_solver(args.folders.solver_dump);
    }
  }
  else if (auto ipdr = get_cref<algo::t_IPDR>(args.algorithm))
  {
    if (args.experiment)
    {
      pdr::pebbling::experiments::PebblingExperiment exp(args, pebbling, log);
      exp.run();
    }
    else
    {

      pdr::pebbling::IPDR ipdr_algo(context, pebbling, args, log);
      pdr::pebbling::PebblingResult result =
          ipdr_algo.run(ipdr->get().type, false);

      args.folders.trace_file << result.end_result() << endl
                              << result.summary_table() << endl
                              << std::string(20, '=') << endl
                              << result.all_traces() << endl;

      ipdr_algo.internal_alg().show_solver(args.folders.solver_dump);
    }
  }
  else
  {
    assert(std::holds_alternative<algo::t_Bounded>(args.algorithm));
    bounded::BoundedPebbling obj(G, args);
    obj.find_for(G.nodes.size());
  }
}

void handle_peterson(
    model_t::Peterson const& descr, ArgumentList& args, pdr::Logger& log)
{
  using namespace pdr::peterson;
  using my::variant::get_cref;
  using std::endl;

  PetersonModel peter(descr.start, descr.max);
  log.stats.is_peter(descr.start, descr.max);
  peter.show(args.folders.model_file);

  pdr::Context context =
      std::holds_alternative<bool>(args.r_seed)
          ? pdr::Context(peter, std::get<bool>(args.r_seed))
          : pdr::Context(peter, std::get<unsigned>(args.r_seed));

  if (args.z3pdr)
  {
    std::cerr << "z3 pdr peterson test" << std::endl;
    std::cerr << "not implemented" << std::endl;
    std::cerr << "ignoring other arguments" << std::endl;
    return;
  }

  if (std::holds_alternative<algo::t_PDR>(args.algorithm))
  {
    pdr::PDR pdr_algo(context, peter, log);

    pdr::PdrResult r = pdr_algo.run();
    {
      tabulate::Table T;
      {
        T.add_row(
            { pdr::PdrResult::fields.cbegin(), pdr::PdrResult::fields.cend() });
        auto const row2 = r.listing();
        T.add_row({ row2.cbegin(), row2.cend() });
      }
      args.folders.trace_file << T << endl << endl;
      args.folders.trace_file << pdr::result::trace_table(r, peter);
      pdr_algo.show_solver(args.folders.solver_dump);
    }
  }
  else if (auto ipdr = get_cref<algo::t_IPDR>(args.algorithm))
  {
    if (args.experiment)
    {
      experiments::PetersonExperiment exp(args, peter, log);
      exp.run();
    }
    else
    {
      using std::endl;
      std::vector<std::unique_ptr<pdr::IpdrResult>> test;

      IPDR ipdr_algo(context, peter, args, log);
      PetersonResult result = ipdr_algo.run(ipdr->get().type, false);

      args.folders.trace_file << result.end_result() << endl
                              << result.summary_table() << endl
                              << std::string('=', 20) << endl
                              << result.all_traces() << endl;

      ipdr_algo.internal_alg().show_solver(args.folders.solver_dump);
    }
  }
  assert(false && "TODO: peterson control flow");
}

////////////////////////////////////////////////////////////////////////////////

void show_peter_model(
    fs::path const& out, pdr::peterson::PetersonModel const& model)
{
  auto model_logger = spdlog::basic_logger_st("model_dump", out, true);
  model_logger->set_level(spdlog::level::trace);
  spdlog::set_pattern("%v");

  tabulate::Table t;
  t.add_row({ "I", model.get_initial().to_string() });
  t.add_row(
      { "P", model.property().to_string(), model.property.p().to_string() });
  t.add_row({ "!P", model.n_property().to_string(),
      model.n_property.p().to_string() });
  // t.add_row({ "T", model.get_constraint().to_string() });
  SPDLOG_LOGGER_TRACE(model_logger, t.str());
  model_logger->flush();
}

#warning dont cares (?) in trace for non-tseytin. dont always make sense? mainly in high constraints
int main(int argc, char* argv[])
{
  using my::variant::get_cref;

  ArgumentList args(argc, argv);


  if (args.onlyshow)
    return 0;

  std::cout << args.folders.trace_file.is_open() << std::endl;

  args.show_header(std::cerr);
  args.folders.show(std::cerr);

  pdr::Logger logger = pdr::Logger(args.folders.file_in_analysis("log"),
      args.out, args.verbosity,
      pdr::Statistics(args.folders.file_in_analysis("stats")));

  if (auto peb_descr = get_cref<model_t::Pebbling>(args.model))
    handle_pebbling(peb_descr->get(), args, logger);
  else
  {
    using namespace pdr::peterson;
    auto const& peter_descr = std::get<model_t::Peterson>(args.model);
    handle_peterson(peter_descr, args, logger);
  }

  std::cout << "goodbye :)" << std::endl;
  return 0;
}
