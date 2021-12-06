#ifndef PDR_LOGGER_H
#define PDR_LOGGER_H

#include "dag.h"
#include "stats.h"

#include <fstream>
#include <memory>
#include <ostream>
#include <spdlog/logger.h>
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

      stats.model.emplace("nodes", G.nodes.size());
      stats.model.emplace("edges", G.edges.size());
      stats.model.emplace("outputs", G.output.size());
    }

    // output a message if output is verbose
    std::ostream& out()
    {
      if (level == OutLvl::verbose)
        return _out;
      return null;
    }

    // output an important update if it is not silenced
    std::ostream& whisper() 
    {
      if (level != OutLvl::silent)
        return _out;
      return null;
    }
  };
} // namespace pdr
#endif // LOGGER_H
