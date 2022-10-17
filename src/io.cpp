#include <fmt/core.h>
#include <ghc/filesystem.hpp>
#include <string>
#include <tabulate/table.hpp>
#include <type_traits>

#include "cli-parse.h"
#include "io.h"
#include "tactic.h"

namespace my::io
{
  using namespace pdr::tactic;
  using fmt::format;
  using std::string;

  FolderStructure::FolderStructure(const cli::ArgumentList& args)
  {
    run_type_dir = base_out() / (args.exp_sample ? "experiments" : "runs");

    model_type_dir = run_type_dir;
    {
      if (args.bounded)
        model_type_dir /= "bounded";
      else if (args.peter)
        model_type_dir /= "peter";
      else
        model_type_dir /= "pebbling";
    }

#warning todo: formal mechanism for peterston name
    model_dir = model_type_dir / args.model_name;

    run_dir = model_dir / run_folder_name(args);

    analysis = run_dir / "analysis";

    fs::create_directories(analysis);

    file_base = file_name(args);
  }

  const FolderStructure FolderStructure::make_from(
      const cli::ArgumentList& args)
  {
    return FolderStructure(args);
  }

  void FolderStructure::show(std::ostream& out) const
  {
    tabulate::Table t;
    t.add_row({ "output directory", run_dir.string() });
    t.add_row({ "logs", analysis.string() });
    out << t << std::endl;
  }

  fs::path FolderStructure::file(std::string_view extension) const
  {
    return fmt::format("{}.{}", file_base, extension);
  }

  // AUX
  //

  fs::path base_out() { return setup(fs::current_path() / "output"); }

  string file_name(const ArgumentList& args)
  {
    string file_string =
        format("{}-{}", args.model_name, to_string(args.tactic));

    if (!args.exp_sample && args.starting_value)
      file_string += format("-{}", args.starting_value.value());

    if (args.delta)
      file_string += "-delta";

    return file_string;
  }

  string run_folder_name(const ArgumentList& args)
  {
    string folder_string = to_string(args.tactic);

    if (args.exp_sample)
      folder_string += format("-exp{}", *args.exp_sample);
    else if (args.starting_value)
      folder_string += format("-{}", args.starting_value.value());

    if (args.delta)
      folder_string += "-delta";

    return folder_string;
  }

  fs::path create_model_dir(const ArgumentList& args)
  {
    fs::path results_folder = base_out();
    if (args.exp_sample)
      results_folder /= "experiments";

    if (args.bounded)
      results_folder /= "bounded";
    else if (args.peter)
      results_folder /= "peter";
    else
      results_folder /= "pebbling";

    return setup(results_folder / args.model_name);
  }

  fs::path setup(fs::path p)
  {
    fs::create_directories(p);
    return p;
  }

  fs::path setup_run_path(const ArgumentList& args)
  {
    return setup(
        base_out() / "results" / args.model_name / run_folder_name(args));
  }

  std::ofstream trunc_file(const fs::path& path)
  {
    std::ofstream stream(
        path.string(), std::fstream::out | std::fstream::trunc);
    assert(stream.is_open());
    return stream;
  }
  std::ofstream trunc_file(const fs::path& folder, const std::string& filename,
      const std::string& ext)
  {
    return trunc_file(folder / fmt::format("{}.{}", filename, ext));
  }
} // namespace my::io
