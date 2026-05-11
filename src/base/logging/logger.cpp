#include <fluxeem/base/logging/logger.h>
#include "loguru.hpp"
#include <cstdarg>
#include <memory>
#include <iostream>

namespace fluxeem
{
    Logger::Logger() : level_(LogLevelType::LOG_INFO)
    {
        int argc = 1;
        char *argv[] = {const_cast<char*>(""), nullptr};
        loguru::init(argc, argv);
        LOG_F(INFO, "Fluxeem SDK Version: %s", PROJECT_VERSION);
        
    }

    Logger::~Logger()
    {
        if (log_file_enabled_)
        {
            loguru::remove_all_callbacks();
        }
    }

    void Logger::setLogLevel(LogLevelType level)
    {
        level_ = level;
        loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
    }

    bool Logger::setLogFile(const std::string &log_file)
    {
        if(log_file.empty())
        {
            return false;
        }

        log_file_enabled_ = loguru::add_file(log_file.c_str(), loguru::Append, loguru::g_stderr_verbosity);
        return log_file_enabled_;
    }

    // Helper: enable thread/file preamble fields only for WARNING and above.
    static void applyPreamblePolicy(loguru::Verbosity verbosity)
    {
        const bool show_detail = (verbosity <= loguru::Verbosity_WARNING); // WARNING, ERROR, FATAL
        loguru::g_preamble_thread = show_detail;
        loguru::g_preamble_file   = show_detail;
    }

    void Logger::log_(LogLevelType level, const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        loguru::Verbosity verbosity;
        switch (level)
        {
        case LogLevelType::LOG_FATAL:   verbosity = loguru::Verbosity_FATAL;   break;
        case LogLevelType::LOG_ERROR:   verbosity = loguru::Verbosity_ERROR;   break;
        case LogLevelType::LOG_WARNING: verbosity = loguru::Verbosity_WARNING; break;
        case LogLevelType::LOG_INFO:    verbosity = loguru::Verbosity_INFO;    break;
        case LogLevelType::LOG_DEBUG:   verbosity = static_cast<loguru::Verbosity>(1); break;
        default:                        verbosity = loguru::Verbosity_INFO;    break;
        }
        applyPreamblePolicy(verbosity);
        loguru::log(verbosity, file, static_cast<unsigned>(line), "%s", buffer);
    }

    void Logger::info(const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        applyPreamblePolicy(loguru::Verbosity_INFO);
        loguru::log(loguru::Verbosity_INFO, file, static_cast<unsigned>(line), "%s", buffer);
    }

    void Logger::warn(const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        applyPreamblePolicy(loguru::Verbosity_WARNING);
        loguru::log(loguru::Verbosity_WARNING, file, static_cast<unsigned>(line), "%s", buffer);
    }

    void Logger::error(const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        applyPreamblePolicy(loguru::Verbosity_ERROR);
        loguru::log(loguru::Verbosity_ERROR, file, static_cast<unsigned>(line), "%s", buffer);
    }

    void Logger::debug(const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        applyPreamblePolicy(static_cast<loguru::Verbosity>(1));
        loguru::log(static_cast<loguru::Verbosity>(1), file, static_cast<unsigned>(line), "%s", buffer);
    }

    void Logger::fatal(const char *file, int line, const char *fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        applyPreamblePolicy(loguru::Verbosity_FATAL);
        loguru::log(loguru::Verbosity_FATAL, file, static_cast<unsigned>(line), "%s", buffer);
    }
}
