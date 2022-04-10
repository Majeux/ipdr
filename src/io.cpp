#include <string>

#include "cli-parse.h"
#include "tactic.h"
#include "io.h"

namespace my::io
{
  using cli::ArgumentList;
  using namespace pdr::tactic;
  namespace fs = ghc::filesystem;

  std::string file_name(const ArgumentList& args)
  {
    std::string file_string =
        fmt::format("{}-{}", args.model_name, to_string(args.tactic));

    if (!(args.tactic == pdr::Tactic::increment ||
            args.tactic == pdr::Tactic::decrement))
      file_string += fmt::format("-{}", args.max_pebbles.value());
    if (args.delta)
      file_string += "-delta";

    return file_string;
  }

  std::string folder_name(const ArgumentList& args)
  {
    std::string folder_string = to_string(args.tactic);

    if (!(args.tactic == pdr::Tactic::increment ||
            args.tactic == pdr::Tactic::decrement))
      folder_string += fmt::format("-{}", args.max_pebbles.value());
    if (args.delta)
      folder_string += "-delta";

    return folder_string;
  }

  fs::path setup_model_path(const ArgumentList& args)
  {
    fs::path results_folder = fs::current_path() / "output";
    fs::create_directory(results_folder);
    fs::path model_dir = results_folder / args.model_name;
    fs::create_directory(model_dir);

    return model_dir;
  }

  fs::path setup_run_path(const ArgumentList& args)
  {
    fs::path results_folder = fs::current_path() / "output";
    fs::create_directory(results_folder);
    fs::path run_dir = results_folder / args.model_name / folder_name(args);
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
