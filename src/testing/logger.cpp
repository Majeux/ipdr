#include "logger.h"
#include "io.h"
#include "stats.h"
#include <spdlog/spdlog.h>

namespace pdr
{
  using my::io::trunc_file;

  Logger::Logger(const std::string& log_file, OutLvl l, Statistics&& s)
      : _out(std::cout), stats(std::move(s)), level(l)
  {
    init(log_file);
  }

  Logger::Logger(const std::string& log_file, const std::string& pfilename,
      OutLvl l, Statistics&& s)
      : progress_file(trunc_file(pfilename)), _out(progress_file),
        stats(std::move(s)), level(l)
  {
    init(log_file);
    if (!progress_file.is_open())
      throw std::runtime_error("Failed to open " + std::string(pfilename));
  }

  void Logger::init(const std::string& log_file)
  {
    // log file truncates
    spd_logger = spdlog::basic_logger_mt("pdr_logger", log_file, true);
    spdlog::set_default_logger(spd_logger);
    spd_logger->set_level(spdlog::level::trace);
    // spdlog::flush_every(std::chrono::seconds(20));
    spdlog::flush_on(spdlog::level::trace);
    // spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%# %!()]");
    // spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%#]");
    spdlog::set_pattern("[%C-%m-%d %T:%e] %v");
  }
} // namespace pdr
