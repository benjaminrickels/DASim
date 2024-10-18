#ifndef DASIM__LOG_H
#define DASIM__LOG_H

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#define DASIM_LOGLEVEL_TRACE   6
#define DASIM_LOGLEVEL_DEBUG   5
#define DASIM_LOGLEVEL_INFO    4
#define DASIM_LOGLEVEL_WARNING 3
#define DASIM_LOGLEVEL_ERROR   2
#define DASIM_LOGLEVEL_FATAL   1

#ifndef DASIM_LOGLEVEL
#define DASIM_LOGLEVEL DASIM_LOGLEVEL_TRACE
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#ifndef NDASIM_LOG
#define DASIM_LOG(LEVEL, MSG, ...)                                                                 \
    do {                                                                                           \
        using namespace std::string_literals;                                                      \
        auto fstr = "[%s] %s() (%s:%u): "s + MSG + "\n"s;                                          \
        fprintf(stderr, fstr.c_str(), LEVEL, __FUNCTION__, __FILE_NAME__, __LINE__,                \
                ##__VA_ARGS__);                                                                    \
    } while (0)
#else
#define DASIM_LOG(LEVEL, ...) ((void)0)
#endif

#define POSIX_ERRSTR (std::strerror(errno))

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_TRACE)
#define DASIM_LOG_TRACE(MSG, ...) ((void)0)
#else
#define DASIM_LOG_TRACE(MSG, ...) DASIM_LOG("DASIM TRACE", MSG, ##__VA_ARGS__)
#endif

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_DEBUG)
#define DASIM_LOG_DEBUG(MSG, ...) ((void)0)
#else
#define DASIM_LOG_DEBUG(MSG, ...) DASIM_LOG("DASIM DEBUG", MSG, ##__VA_ARGS__)
#endif

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_INFO)
#define DASIM_LOG_INFO(MSG, ...) ((void)0)
#else
#define DASIM_LOG_INFO(MSG, ...) DASIM_LOG("DASIM INFO", MSG, ##__VA_ARGS__)
#endif

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_WARNING)
#define DASIM_LOG_WARNING(MSG, ...) ((void)0)
#else
#define DASIM_LOG_WARNING(MSG, ...) DASIM_LOG("DASIM WARNING", MSG, ##__VA_ARGS__)
#endif

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_ERROR)
#define DASIM_LOG_ERROR(MSG, ...) ((void)0)
#else
#define DASIM_LOG_ERROR(MSG, ...) DASIM_LOG("DASIM ERROR", MSG, ##__VA_ARGS__)
#endif

#if (DASIM_LOGLEVEL < DASIM_LOGLEVEL_FATAL)
#define DASIM_LOG_FATAL(MSG, ...) ((void)0)
#else
#define DASIM_LOG_FATAL(MSG, ...) DASIM_LOG("DASIM FATAL", MSG, ##__VA_ARGS__)
#endif

#define DASIM_TRACE(MSG, ...)   DASIM_LOG_TRACE(MSG, ##__VA_ARGS__)
#define DASIM_DEBUG(MSG, ...)   DASIM_LOG_DEBUG(MSG, ##__VA_ARGS__)
#define DASIM_INFO(MSG, ...)    DASIM_LOG_INFO(MSG, ##__VA_ARGS__)
#define DASIM_WARNING(MSG, ...) DASIM_LOG_WARNING(MSG, ##__VA_ARGS__)
#define DASIM_ERROR(MSG, ...)   DASIM_LOG_ERROR(MSG, ##__VA_ARGS__)
#define DASIM_FATAL(MSG, ...)                                                                      \
    do {                                                                                           \
        DASIM_LOG_FATAL(MSG, ##__VA_ARGS__);                                                       \
        std::abort();                                                                              \
    } while (0)

#endif
