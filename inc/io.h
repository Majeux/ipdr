#ifndef IO_H
#define IO_H

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace my::io
{
  namespace fs = std::filesystem;

  const fs::path BENCH_FOLDER = fs::current_path();

  fs::path base_out();
  fs::path in_root(fs::path const& p);
  fs::path setup(fs::path p);
  const fs::path file_in(fs::path const& folder, std::string_view name,
      std::string_view extension);
  // creates an overwriting file of the given name
  std::ofstream trunc_file(fs::path const& path);
  // creates it with the the given extension, in the given folder.
  std::ofstream trunc_file(fs::path const& folder, std::string const& filename,
      std::string const& ext);

  // run_type_dir / model_type_dir / model_dir / run_dir / run_files
  // ex: output / experiments / ipdr / pebbling / ham3tc /
  // ham3tc-ipdr_constrain-exp10
  struct FolderStructure
  {
    fs::path bench_src;
    fs::path run_type_dir;   // (experiments | runs) / (pdr | ipdr | bounded)
    fs::path model_type_dir; // pebbling or peterson
    fs::path model_dir;      // model_name
    fs::path run_dir;        // modeltag - algotag - experiment?
    fs::path analysis;
    std::string file_base;

    std::ofstream trace_file;
    std::ofstream solver_dump;
    std::ofstream model_file;

    void show(std::ostream& out) const;
    fs::path file(std::string_view name, std::string_view extension) const;
    fs::path file(std::string_view extension) const;
    fs::path file_in_model(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_model(std::string_view extension) const;
    fs::path file_in_run(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_run(std::string_view extension) const;
    fs::path file_in_analysis(
        std::string_view name, std::string_view extension) const;
    fs::path file_in_analysis(std::string_view extension) const;
    fs::path src_file(std::string_view name, std::string_view extension) const;
  };
} // namespace my::io
#endif // IO_H
