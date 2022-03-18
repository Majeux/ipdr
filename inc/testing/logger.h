#ifndef PDR_LOGGER_H
#define PDR_LOGGER_H

#include "dag.h"
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

namespace pdr
{
  class Logger
  {
   private:
    std::ofstream progress_file;
    std::ostream& _out;
    nullstream null;

   public:
    std::shared_ptr<spdlog::logger> spd_logger;
    Statistics stats;
    OutLvl level;
    unsigned indent = 0;

    std::string tab() const { return std::string(indent, '\t'); }

    Logger(const std::string& log_file, const dag::Graph& G, OutLvl l)
        : _out(std::cout), stats(), level(l)
    {
      init(log_file, G);
    }

    Logger(const std::string& log_file, const dag::Graph& G,
           const std::string& pfilename, OutLvl l)
        : progress_file(pfilename, std::fstream::out | std::fstream::trunc),
          _out(progress_file), stats(), level(l)
    {
      init(log_file, G);
      if (!progress_file.is_open())
        throw std::runtime_error("Failed to open " + std::string(pfilename));
    }

    void init(const std::string& log_file, const dag::Graph& G)
    {
      spd_logger = spdlog::basic_logger_mt("pdr_logger", log_file);
      spd_logger->set_level(spdlog::level::trace);
      // spdlog::flush_every(std::chrono::seconds(20));
      spdlog::flush_on(spdlog::level::trace);
      // spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%# %!()]");
      spdlog::set_pattern("[%C-%m-%d %T:%e] %v \n  @[%s:%#]");

      stats.model.emplace("nodes", G.nodes.size());
      stats.model.emplace("edges", G.edges.size());
      stats.model.emplace("outputs", G.output.size());
    }

    // LOGGING OUTPUT
    //
    // log a message
    template <typename... Args>
    void operator()(std::string_view message_fmt, Args&&... a)
    {
      SPDLOG_LOGGER_TRACE(spd_logger, message_fmt, std::forward<Args>(a)...);
    }

    template <typename... Args>
    void log(std::string_view message_fmt, Args&&... a)
    {
      operator()(message_fmt, std::forward<Args>(a)...);
    }

    // log a message with indent
    template <typename... Args>
    void tabbed(const std::string& message_fmt, Args&&... a)
    {
      std::string fmt = "{}| " + message_fmt;
      SPDLOG_LOGGER_TRACE(spd_logger, fmt, tab(), std::forward<Args>(a)...);
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
    void and_show(std::string_view message,
                  Args&&... a) // TODO rename to operator
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
  };
} // namespace pdr
#endif // LOGGER_H
