#pragma once
#include "../common/Common.h"

namespace DistributedChat {
    enum class LogLevel {
        INFO_LEVEL,
        SUCCESS_LEVEL,
        WARN_LEVEL,
        ERR_LEVEL,
        DEBUG_LEVEL
    };

    class Logger {
    private:
        static std::mutex s_logMutex;
        static std::ofstream s_logFile;
        static bool s_fileLoggingEnabled;
        static LogLevel s_minLogLevel;

        static std::string LogLevelToString(LogLevel level);
        static std::string LogLevelToColor(LogLevel level);

    public:
        static void Init(const std::string& filepath, LogLevel minLevel = LogLevel::INFO_LEVEL);
        static void Log(LogLevel level, const std::string& message, const std::string& category = "SYSTEM");
        
        static void Info(const std::string& msg, const std::string& cat = "SYSTEM") { Log(LogLevel::INFO_LEVEL, msg, cat); }
        static void Success(const std::string& msg, const std::string& cat = "SYSTEM") { Log(LogLevel::SUCCESS_LEVEL, msg, cat); }
        static void Warn(const std::string& msg, const std::string& cat = "SYSTEM") { Log(LogLevel::WARN_LEVEL, msg, cat); }
        static void Error(const std::string& msg, const std::string& cat = "SYSTEM") { Log(LogLevel::ERR_LEVEL, msg, cat); }
        static void Debug(const std::string& msg, const std::string& cat = "SYSTEM") { Log(LogLevel::DEBUG_LEVEL, msg, cat); }
    };
}