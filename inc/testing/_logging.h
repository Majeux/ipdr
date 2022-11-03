#ifndef LOGGING_H
#define LOGGING_H

#include <memory>
#ifdef LOG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#define SPDLOG_LEVEL_NAMES { "[trace]   ", "[debug]   ", "[info]    ", "[warning] ", "[error]   ", "[critical]", "[     ]" }`
#else 
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif //LOG

#include <spdlog/spdlog.h>

#endif //LOGGING_H
