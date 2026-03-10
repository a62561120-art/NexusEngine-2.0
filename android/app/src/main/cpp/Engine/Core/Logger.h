#pragma once
#include <string>
#include <android/log.h>
#include <sstream>

#include <ctime>

namespace Nova {

enum class LogLevel {
    Trace = 0,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void SetLevel(LogLevel level) { m_level = level; }
    LogLevel GetLevel() const { return m_level; }

    void Log(LogLevel level, const std::string& category, const std::string& msg) {
        if (level < m_level) return;
        std::string prefix = LevelPrefix(level);
        std::string output = "[" + prefix + "][" + category + "] " + msg;
        if (level >= LogLevel::Error)
            __android_log_print(ANDROID_LOG_ERROR, "NexusEngine", "%s", output.c_str());
        else
            __android_log_print(ANDROID_LOG_INFO, "NexusEngine", "%s", output.c_str());
    }

    void Trace(const std::string& cat, const std::string& msg)   { Log(LogLevel::Trace,   cat, msg); }
    void Info(const std::string& cat, const std::string& msg)    { Log(LogLevel::Info,    cat, msg); }
    void Warn(const std::string& cat, const std::string& msg)    { Log(LogLevel::Warning, cat, msg); }
    void Error(const std::string& cat, const std::string& msg)   { Log(LogLevel::Error,   cat, msg); }
    void Fatal(const std::string& cat, const std::string& msg)   { Log(LogLevel::Fatal,   cat, msg); }

private:
    Logger() : m_level(LogLevel::Info) {}

    std::string LevelPrefix(LogLevel l) {
        switch (l) {
            case LogLevel::Trace:   return "TRACE";
            case LogLevel::Info:    return "INFO ";
            case LogLevel::Warning: return "WARN ";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::Fatal:   return "FATAL";
            default:                return "?    ";
        }
    }

    LogLevel m_level;
};

// Convenience macros
#define NOVA_LOG_TRACE(cat, msg) ::Nova::Logger::Get().Trace(cat, msg)
#define NOVA_LOG_INFO(cat, msg)  ::Nova::Logger::Get().Info(cat, msg)
#define NOVA_LOG_WARN(cat, msg)  ::Nova::Logger::Get().Warn(cat, msg)
#define NOVA_LOG_ERROR(cat, msg) ::Nova::Logger::Get().Error(cat, msg)
#define NOVA_LOG_FATAL(cat, msg) ::Nova::Logger::Get().Fatal(cat, msg)

// Helper to build log strings
template<typename... Args>
std::string LogFmt(Args&&... args) {
    std::ostringstream oss;
    (oss << ... << args);
    return oss.str();
}

} // namespace Nova
