#include <ghc/filesystem.hpp>
#include <string>

#include "cli-parse.h"
#include "io.h"
#include "tactic.h"

namespace my::io
{
  using namespace pdr::tactic;
  using fmt::format;
  using std::string;

  fs::path base_out()
  {
    fs::path rv = fs::current_path() / "output";
    fs::create_directory(rv);
    return rv;
  }

  string file_name(const ArgumentList& args)
  {
    string file_string =
        format("{}-{}", args.model_name, to_string(args.tactic));

    if (!(args.tactic == pdr::Tactic::increment ||
            args.tactic == pdr::Tactic::decrement) && !args.bounded)
      file_string += format("-{}", args.starting_value.value());
    if (args.delta)
      file_string += "-delta";

    return file_string;
  }

  string folder_name(const ArgumentList& args)
  {
    string folder_string = to_string(args.tactic);

    if (args.exp_sample)
    {
      folder_string += "-";
      if (args.exp_control)
        folder_string += "C";
      folder_string += format("exp{}", *args.exp_sample);
    }

    if (!(args.tactic == pdr::Tactic::increment ||
            args.tactic == pdr::Tactic::decrement) && !args.bounded)
      folder_string += format("-{}", args.starting_value.value());
    if (args.delta)
      folder_string += "-delta";

    return folder_string;
  }

  fs::path setup_model_path(const ArgumentList& args)
  {
    fs::path results_folder = base_out();
    if (args.bounded)
      results_folder /= "bounded";
    else if (args.exp_sample)
      results_folder /= "experiments";
    else
      results_folder /= "results";

    fs::create_directory(results_folder);
    fs::path model_dir = results_folder / args.model_name;
    fs::create_directory(model_dir);

    return model_dir;
  }

  fs::path setup_path(fs::path p)
  {
    fs::create_directory(p);
    return p;
  }

  fs::path setup_run_path(const ArgumentList& args)
  {
    fs::path results_folder = base_out() / "results";
    fs::create_directory(results_folder);
    fs::path run_dir = results_folder / args.model_name / folder_name(args);
    fs::create_directory(run_dir);

    return run_dir;
  }

  fs::path setup_exp_path(const ArgumentList& args)
  {
    assert(args.exp_sample);
    fs::path exp_folder = base_out() / "experiments";
    fs::create_directory(exp_folder);
    fs::path run_dir = exp_folder / args.model_name / folder_name(args);
    fs::create_directory(run_dir);

    return run_dir;
  }

  std::ofstream trunc_file(const fs::path& folder, const std::string& filename,
      const std::string& ext)
  {
    fs::path file = folder / fmt::format("{}.{}", filename, ext);
    std::ofstream stream(
        file.string(), std::fstream::out | std::fstream::trunc);
    assert(stream.is_open());
    return stream;
  }
} // namespace my::io
