#include "logger.h"
#include "io.h"
#include "stats.h"
#include <spdlog/spdlog.h>

namespace pdr
{
  using my::io::trunc_file;

  Logger::Logger(const std::string& log_file,
      std::optional<std::string_view> pfilename, OutLvl l, Statistics&& s)
      : _out(progress_file), stats(std::move(s)), level(l)
  {
    if (pfilename)
    {
      progress_file = trunc_file(*pfilename);
      if (!progress_file.is_open())
        throw std::runtime_error("Failed to open " + std::string{ *pfilename });
    }

    init(log_file);
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
    spdlog::set_pattern("%T [%=8l] %v");
  }
} // namespace pdr
