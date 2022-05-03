#include "logger.h"

namespace pdr
{
  Logger::Logger(const std::string& log_file, const dag::Graph& G, OutLvl l,
      std::ofstream&& stat_file)
      : _out(std::cout), stats(std::move(stat_file), G), level(l)
  {
    init(log_file);
  }

  Logger::Logger(const std::string& log_file, const dag::Graph& G,
      const std::string& pfilename, OutLvl l, std::ofstream&& stat_file)
      : progress_file(pfilename, std::fstream::out | std::fstream::trunc),
        _out(progress_file), stats(std::move(stat_file), G), level(l)
  {
    init(log_file);
    if (!progress_file.is_open())
      throw std::runtime_error("Failed to open " + std::string(pfilename));
  }

  void Logger::init(const std::string& log_file)
  {
#warning log file truncates
    spd_logger = spdlog::basic_logger_mt("pdr_logger", log_file, true);
    spd_logger->set_level(spdlog::level::trace);
    // spdlog::flush_every(std::chrono::seconds(20));
    spdlog::flush_on(spdlog::level::trace);
    // spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%# %!()]");
    // spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%#]");
    spdlog::set_pattern("[%C-%m-%d %T:%e] %v");
  }
} // namespace pdr
