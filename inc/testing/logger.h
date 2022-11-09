#ifndef PDR_LOGGER_H
#define PDR_LOGGER_H

#include "_logging.h"
#include "stats.h"

#include <dbg.h>
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <string>

enum OutLvl
{
  silent,  // no output
  whisper, // output some
  verbose, // output all
};

class nullbuffer : public std::streambuf
{
 public:
  int overflow(int c) { return c; }
};

class nullstream : public std::ostream
{
 public:
  nullstream() : std::ostream(&null) {}

 private:
  nullbuffer null;
};

// to discard parameter list when logging is turned off
// https://stackoverflow.com/questions/19532475/casting-a-variadic-parameter-pack-to-void
struct sink
{
  template <typename... Args> sink(Args const&...) {}
};

namespace pdr
{
  class Logger
  {
   private:
    std::ofstream progress_file;
    std::ostream& _out;
    nullstream null;

    inline const static std::string_view tabsep = "{}| ";

    std::string tab() const { return std::string(indent, '\t'); }

   public:
    std::shared_ptr<spdlog::logger> spd_logger;
    Statistics stats;
    OutLvl level;
    unsigned indent = 0;

    std::string tabbed(std::string_view msg) const
    {
      return (std::string(2 * indent, ' ') + "| ").append(msg);
    }

    Logger(const std::string& log_file,
        std::optional<std::string_view> pfilename, OutLvl l, Statistics&& s);

    void init(const std::string& log_file);

    // LOGGING OUTPUT
    //
    // log a message
    template <typename... Args>
    void operator()(std::string_view message_fmt, Args&&... a)
    {
      indented(message_fmt, std::forward<Args>(a)...);
    }

    template <typename... Args>
    void untabbed(std::string_view message_fmt, Args&&... a)
    {
      (void)message_fmt;
      sink{ std::forward<Args>(a)... };
      SPDLOG_LOGGER_DEBUG(spd_logger, message_fmt, std::forward<Args>(a)...);
    }

    // log a message with indent
    template <typename... Args>
    void indented(std::string_view message_fmt, Args&&... a)
    {
      (void)message_fmt;
      sink{ std::forward<Args>(a)... };
      SPDLOG_LOGGER_DEBUG(spd_logger, std::string{ tabsep }.append(message_fmt),
          tab(), std::forward<Args>(a)...);
    }

    template <typename... Args>
    void tabbed_trace(std::string_view message_fmt, Args&&... a)
    {
      (void)message_fmt;
      sink{ std::forward<Args>(a)... };
      SPDLOG_LOGGER_TRACE(spd_logger,
          (std::string{ tabsep }.append(message_fmt)), tab(),
          std::forward<Args>(a)...);
    }

    // NON-LOGGING OUTPUT
    //
    // output a verbose message (does not log)
    template <typename... Args>
    void show(std::string_view message_fmt, Args&&... a)
    {
      if (level == OutLvl::verbose)
        _out << fmt::format(message_fmt, std::forward<Args>(a)...) << std::endl;
    }

    // output non-verbose message. only supressed if silent
    template <typename... Args>
    void whisper(std::string_view message_fmt, Args&&... a)
    {
      if (level != OutLvl::silent)
        _out << fmt::format(message_fmt, std::forward<Args>(a)...) << std::endl;
    }

    // COMBINED
    //
    // output a verbose message and log it
    template <typename... Args>
    void and_show(std::string_view message, Args&&... a)
    {
      show(message, std::forward<Args>(a)...);
      SPDLOG_LOGGER_DEBUG(spd_logger, message, std::forward<Args>(a)...);
    }

    template <typename... Args> void warn(std::string_view message, Args&&... a)
    {
      show(message, std::forward<Args>(a)...);
      SPDLOG_LOGGER_WARN(spd_logger, message, std::forward<Args>(a)...);
    }

    // output a whisper message and log it
    template <typename... Args>
    void and_whisper(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      whisper(message, std::forward<Args>(a)...);
      SPDLOG_LOGGER_DEBUG(spd_logger, message, std::forward<Args>(a)...);
    }

    template <typename... Args>
    void tabbed_and_show(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      show(message, std::forward<Args>(a)...);
      indented(message, std::forward<Args>(a)...);
    }

    // output a whisper message and log it
    template <typename... Args>
    void tabbed_and_whisper(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      whisper(message, std::forward<Args>(a)...);
      indented(message, std::forward<Args>(a)...);
    }
  };
} // namespace pdr

#define SEPWITH 15
#define SEP1 std::string(SEPWITH, '-')
#define SEP2 std::string(SEPWITH, '=')
#define SEP3 std::string(SEPWITH, '#')

#warning todo: make show and whisper macros to avoid computation of arguments

#define MYLOG_WARN(logger, format, ...)  \
  logger.whisper(format, ##__VA_ARGS__); \
  SPDLOG_LOGGER_WARN(logger.spd_logger, logger.tabbed(format), ##__VA_ARGS__)

#define MYLOG_INFO(logger, format, ...)  \
  logger.whisper(format, ##__VA_ARGS__); \
  SPDLOG_LOGGER_INFO(logger.spd_logger, logger.tabbed(format), ##__VA_ARGS__)

#define MYLOG_DEBUG(logger, format, ...) \
  SPDLOG_LOGGER_DEBUG(logger.spd_logger, logger.tabbed(format), ##__VA_ARGS__)

#define MYLOG_DEBUG_SHOW(logger, format, ...) \
  logger.show(format, ##__VA_ARGS__);         \
  MYLOG_DEBUG(logger, format, ##__VA_ARGS__)

#define MYLOG_TRACE(logger, format, ...) \
  SPDLOG_LOGGER_TRACE(logger.spd_logger, logger.tabbed(format), ##__VA_ARGS__)

#endif // LOGGER_H
