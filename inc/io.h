#ifndef IO_H
#define IO_H

#include <ghc/filesystem.hpp>
#include <string>
#include <string_view>

namespace my::io
{
  namespace fs = ghc::filesystem;

  // run_type_dir / model_type_dir / model_dir / run_dir / run_files
  struct FolderStructure
  {
    fs::path bench_src;
    fs::path run_type_dir;
    fs::path model_type_dir;
    fs::path model_dir;
    fs::path run_dir;
    fs::path analysis;
    std::string file_base;

    void show(std::ostream& out) const;
    fs::path file(std::string_view name, std::string_view extension) const;
    fs::path file(std::string_view extension) const;
    fs::path file_in_model(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_model(std::string_view extension) const;
    fs::path file_in_run(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_run(std::string_view extension) const;
  };

  // const fs::path BENCH_FOLDER = fs::current_path() / "benchmark" / "rls" /
  // "tfc";
  const fs::path BENCH_FOLDER = fs::current_path();

  fs::path base_out();
  fs::path setup(fs::path p);
  const fs::path file_in(fs::path const& folder, std::string_view name,
      std::string_view extension);
  // creates an overwriting file of the given name
  std::ofstream trunc_file(fs::path const& path);
  // creates it with the the given extension, in the given folder.
  std::ofstream trunc_file(fs::path const& folder, std::string const& filename,
      std::string const& ext);
} // namespace my::io
#endif // IO_H
