#include "../../../include/core/logging/Logger.h"

namespace DistributedChat {
    std::mutex Logger::s_logMutex;
    std::ofstream Logger::s_logFile;
    bool Logger::s_fileLoggingEnabled = false;
    LogLevel Logger::s_minLogLevel = LogLevel::INFO_LEVEL;

    std::string Logger::LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::INFO_LEVEL:    return "INFO";
            case LogLevel::SUCCESS_LEVEL: return "SUCCESS";
            case LogLevel::WARN_LEVEL:    return "WARN";
            case LogLevel::ERR_LEVEL:     return "ERROR";
            case LogLevel::DEBUG_LEVEL:   return "DEBUG";
        }
        return "UNKNOWN";
    }

    std::string Logger::LogLevelToColor(LogLevel level) {
        switch (level) {
            case LogLevel::INFO_LEVEL:    return "\033[94m"; // Bright Blue
            case LogLevel::SUCCESS_LEVEL: return "\033[92m"; // Bright Green
            case LogLevel::WARN_LEVEL:    return "\033[93m"; // Bright Yellow
            case LogLevel::ERR_LEVEL:     return "\033[91m"; // Bright Red
            case LogLevel::DEBUG_LEVEL:   return "\033[95m"; // Bright Magenta
        }
        return "\033[0m";
    }

    void Logger::Init(const std::string& filepath, LogLevel minLevel) {
        std::lock_guard<std::mutex> lock(s_logMutex);
        s_minLogLevel = minLevel;

        // Enable Windows Console Virtual Terminal (ANSI) colors
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }

        s_logFile.open(filepath, std::ios::out | std::ios::app);
        if (s_logFile.is_open()) {
            s_fileLoggingEnabled = true;
        }
    }

    void Logger::Log(LogLevel level, const std::string& message, const std::string& category) {
        if (level < s_minLogLevel) return;

        std::lock_guard<std::mutex> lock(s_logMutex);

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_s(&tm_buf, &in_time_t);

        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_buf);

        std::string levelStr = LogLevelToString(level);
        std::string color = LogLevelToColor(level);
        std::string resetColor = "\033[0m";

        std::stringstream ssConsole;
        ssConsole << "[" << timeStr << "] "
                  << color << "[" << levelStr << "]" << resetColor
                  << " [" << category << "] "
                  << message;

        std::stringstream ssFile;
        ssFile << "[" << timeStr << "] "
               << "[" << levelStr << "]"
               << " [" << category << "] "
               << message;

        std::cout << ssConsole.str() << "\n" << std::flush;

        if (s_fileLoggingEnabled && s_logFile.is_open()) {
            s_logFile << ssFile.str() << "\n" << std::flush;
        }
    }
}