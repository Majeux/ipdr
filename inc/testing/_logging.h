#ifndef LOGGING_H
#define LOGGING_H

#ifdef LOG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else 
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif //LOG

#include <spdlog/spdlog.h>
#define SEP "--------------------"
#define SEP2 "===================="
#define SEP3 "####################"


#endif //LOGGING_H
