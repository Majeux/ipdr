#include <fmt/core.h>
#include <ghc/filesystem.hpp>
#include <string>
#include <string_view>
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

  fs::path FolderStructure::file_in_model(
      std::string_view name, std::string_view extension) const
  {
    return model_dir / format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file_in_model(std::string_view extension) const
  {
    return model_dir / format("{}.{}", file_base, extension);
  }

  fs::path FolderStructure::file_in_run(
      std::string_view name, std::string_view extension) const
  {
    return model_dir / format("{}.{}", name, extension);
  }

  fs::path FolderStructure::file_in_run(std::string_view extension) const
  {
    return model_dir / format("{}.{}", file_base, extension);
  }

  // AUX
  //

  fs::path base_out() { return setup(fs::current_path() / "output"); }

  fs::path setup(fs::path p)
  {
    fs::create_directories(p);
    return p;
  }

  const fs::path file_in(
      const fs::path& folder, std::string_view name, std::string_view extension)
  {
    return base_out() / folder / format("{}.{}", name, extension);
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
    return trunc_file(folder / format("{}.{}", filename, ext));
  }
} // namespace my::io
