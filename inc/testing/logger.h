#ifndef PDR_LOGGER_H
#define PDR_LOGGER_H

#include "stats.h"

#include "_logging.h"
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

enum OutLvl
{
  silent,
  whisper,
  verbose
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

    std::string tab() const { return std::string(indent, '\t'); }

   public:
    std::shared_ptr<spdlog::logger> spd_logger;
    Statistics stats;
    OutLvl level;
    unsigned indent = 0;

    Logger(const std::string& log_file, OutLvl l, Statistics&& s);

    Logger(const std::string& log_file, const std::string& pfilename, OutLvl l,
        Statistics&& s);

    void init(const std::string& log_file);

    // LOGGING OUTPUT
    //
    // log a message
    template <typename... Args>
    void operator()(std::string_view message_fmt, Args&&... a)
    {
      (void)message_fmt;
      sink{ std::forward<Args>(a)... };
      SPDLOG_LOGGER_TRACE(spd_logger, message_fmt, std::forward<Args>(a)...);
    }

    template <typename... Args>
    void log(std::string_view message_fmt, Args&&... a)
    {
      operator()(message_fmt, std::forward<Args>(a)...);
    }

    // log a message with indent
    template <typename... Args>
    void tabbed(std::string_view message_fmt, Args&&... a)
    {
      std::string fmt = "{}| "; // + message_fmt;
      (void)message_fmt;
      sink{ std::forward<Args>(a)... };
      SPDLOG_LOGGER_TRACE(
          spd_logger, fmt.append(message_fmt), tab(), std::forward<Args>(a)...);
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
      SPDLOG_LOGGER_TRACE(spd_logger, message, std::forward<Args>(a)...);
    }

    // output a whisper message and log it
    template <typename... Args>
    void and_whisper(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      whisper(message, std::forward<Args>(a)...);
      SPDLOG_LOGGER_TRACE(spd_logger, message, std::forward<Args>(a)...);
    }

    template <typename... Args>
    void tabbed_and_show(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      show(message, std::forward<Args>(a)...);
      tabbed(message, std::forward<Args>(a)...);
    }

    // output a whisper message and log it
    template <typename... Args>
    void tabbed_and_whisper(std::string_view message,
        Args&&... a) // TODO rename to operator
    {
      whisper(message, std::forward<Args>(a)...);
      tabbed(message, std::forward<Args>(a)...);
    }
  };
} // namespace pdr
#endif // LOGGER_H
