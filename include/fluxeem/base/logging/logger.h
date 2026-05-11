#ifndef __FLUXEEM_LOGGER_H__
#define __FLUXEEM_LOGGER_H__

#include <string>
#include <fluxeem/base/define/base_define.h>
#include <memory>

#define LOGGER fluxeem::Logger::Instance()

#define LOG_INFO(fmt, ...) fluxeem::Logger::Instance().info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fluxeem::Logger::Instance().warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fluxeem::Logger::Instance().error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fluxeem::Logger::Instance().debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) fluxeem::Logger::Instance().fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

namespace fluxeem
{

    enum class FLUXEEM_API LogLevelType
    {
        LOG_INVALID = -10, // Never do LOG_F(INVALID)

        // You may use OFF on g_stderr_verbosity, but for nothing else!
        LOG_OFF = -9, // Never do LOG_F(OFF)

        // Prefer to use ABORT_F or ABORT_S over LOG_F(FATAL) or LOG_S(FATAL).
        LOG_FATAL = -3,
        LOG_ERROR = -2,
        LOG_WARNING = -1,

        // Normal messages. By default written to stderr.
        LOG_INFO = 0,

        LOG_DEBUG = 1,
    };

    class FLUXEEM_API Logger
    {
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;
        ~Logger();

    public:
        void setLogLevel(LogLevelType level);
        LogLevelType getLogLevel() const { return level_; }

        // 静态方法，获取单例实例
        static Logger &Instance()
        {
            static Logger instance;
            return instance;
        }
        bool setLogFile(const std::string &log_file);

        void log_(LogLevelType level, const char *file, int line, const char *fmt, ...);

        // 提供便捷的日志函数
        void info(const char *file, int line, const char *fmt, ...);
        void warn(const char *file, int line, const char *fmt, ...);
        void error(const char *file, int line, const char *fmt, ...);
        void debug(const char *file, int line, const char *fmt, ...);
        void fatal(const char *file, int line, const char *fmt, ...);

    private:
        LogLevelType level_;
        bool log_file_enabled_;
        Logger();
    };

}

#endif // __FLUXEEM_LOGGER_H__
