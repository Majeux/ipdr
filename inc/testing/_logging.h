#ifdef LOG
// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif // LOG

#ifdef STATS
#define IF_STATS(code) code
#else
#define IF_STATS(code)
#endif // STATS

#include <spdlog/spdlog.h>
