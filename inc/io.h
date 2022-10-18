#ifndef IO_H
#define IO_H

#include <ghc/filesystem.hpp>
#include <string>
#include <string_view>

#include "cli-parse.h"

namespace my::io
{
  namespace fs = ghc::filesystem;
  using cli::ArgumentList;

  // run_type_dir / model_type_dir / model_dir / run_dir / run_files
  struct FolderStructure
  {
    fs::path run_type_dir;
    fs::path model_type_dir;
    fs::path model_dir;
    fs::path run_dir;
    fs::path analysis;
    std::string file_base;

    static const FolderStructure make_from(const ArgumentList& args);

    void show(std::ostream& out) const;
    fs::path file(std::string_view name, std::string_view extension) const;
    fs::path file(std::string_view extension) const;
    fs::path file_in_model(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_model(std::string_view extension) const;

   private:
    FolderStructure(const ArgumentList& args);
  };

  // const fs::path BENCH_FOLDER = fs::current_path() / "benchmark" / "rls" /
  // "tfc";
  const fs::path BENCH_FOLDER = fs::current_path();

  fs::path base_out();
  std::string file_name(const ArgumentList& args);
  std::string run_folder_name(const ArgumentList& args);
  fs::path setup(fs::path p);
  fs::path create_model_dir(const ArgumentList& args);
  fs::path setup_run_path(const ArgumentList& args);
  // creates an overwriting file of the given name
  std::ofstream trunc_file(const fs::path& path);
  // creates it with the the given extension, in the given folder.
  std::ofstream trunc_file(const fs::path& folder, const std::string& filename,
      const std::string& ext);
} // namespace my::io
#endif // IO_H
