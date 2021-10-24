#ifndef LOGGER_H
#define LOGGER_H

#include "stats.h"

#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

namespace pdr
{
    struct Logger {
        Statistics stats;
        std::shared_ptr<spdlog::logger> spd_logger;

        unsigned indent = 0;

        std::string tab() const { return std::string(indent, '\t'); }

        Logger(const std::string& name) : stats()
        {
            string log_file = name + ".log";
            spd_logger =
                spdlog::basic_logger_mt("pdr_logger", "logs/" + log_file);
            spd_logger->set_level(spdlog::level::trace);
            // spdlog::flush_every(std::chrono::seconds(20));
            spdlog::flush_on(spdlog::level::trace);
        }
    };
} // namespace pdr
#endif // LOGGER_H
