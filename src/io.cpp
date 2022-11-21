#include "cli-parse.h"
#include "io.h"
#include "tactic.h"

#include <filesystem>
#include <fmt/core.h>
#include <ghc/filesystem.hpp>
#include <string>
#include <string_view>
#include <tabulate/table.hpp>
#include <type_traits>

namespace my::io
{
  using namespace pdr::tactic;
  using fmt::format;
  using std::string;

  // AUX
  //
  fs::path base_out() { return setup(fs::current_path() / "output"); }
  fs::path in_root(fs::path const& p) { return fs::current_path() / p; }

  fs::path setup(fs::path p)
  {
    fs::create_directories(p);
    return p;
  }

  const fs::path file_in(
      fs::path const& folder, std::string_view name, std::string_view extension)
  {
    return base_out() / folder / format("{}.{}", name, extension);
  }

  std::ofstream trunc_file(fs::path const& path)
  {
    std::ofstream stream(
        path.string(), std::fstream::out | std::fstream::trunc);
    assert(stream.is_open());
    return stream;
  }
  std::ofstream trunc_file(fs::path const& folder, std::string const& filename,
      std::string const& ext)
  {
    return trunc_file(folder / format("{}.{}", filename, ext));
  }

  // FOLDERSTRUCTURE
  //
  void FolderStructure::show(std::ostream& out) const
  {
    tabulate::Table t;
    t.add_row({ "output directory", run_dir.string() });
    t.add_row({ "logs", analysis.string() });
    out << t << std::endl;
  }

  fs::path FolderStructure::file(
      std::string_view name, std::string_view extension) const
  {
    return format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file(std::string_view extension) const
  {
    return format("{}.{}", file_base, extension);
  }

  // MODEL
  fs::path FolderStructure::file_in_model(
      std::string_view name, std::string_view extension) const
  {
    return model_dir / format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file_in_model(std::string_view extension) const
  {
    return model_dir / format("{}.{}", file_base, extension);
  }

  // RUN
  fs::path FolderStructure::file_in_run(
      std::string_view name, std::string_view extension) const
  {
    return run_dir / format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file_in_run(std::string_view extension) const
  {
    return run_dir / format("{}.{}", file_base, extension);
  }

  // ANALYSIS
  fs::path FolderStructure::file_in_analysis(
      std::string_view name, std::string_view extension) const
  {
    return analysis / format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file_in_analysis(std::string_view extension) const
  {
    return analysis / format("{}.{}", file_base, extension);
  }

  // BENCH
  fs::path FolderStructure::src_file(
      std::string_view name, std::string_view extension) const
  {
    return fs::current_path() / bench_src / format("{}.{}", name, extension);
  }
} // namespace my::io
