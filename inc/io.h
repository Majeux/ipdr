#ifndef IO_H
#define IO_H

#include <string>

#include "cli-parse.h"

namespace my::io
{
  namespace fs = ghc::filesystem;
  using cli::ArgumentList;

  // const fs::path BENCH_FOLDER = fs::current_path() / "benchmark" / "rls" /
  // "tfc";
  const fs::path BENCH_FOLDER = fs::current_path();

  std::string file_name(const ArgumentList& args);
  std::string folder_name(const ArgumentList& args);
  fs::path setup_model_path(const ArgumentList& args);
  fs::path setup_run_path(const ArgumentList& args);
  std::ofstream trunc_file(const fs::path& folder, const std::string& filename,
      const std::string& ext);
} // namespace my::io
#endif // IO_H
