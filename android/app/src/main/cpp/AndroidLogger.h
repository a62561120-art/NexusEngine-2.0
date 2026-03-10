#pragma once
// Android-safe logger — replaces cout/cerr with __android_log_print
// Include this BEFORE any engine headers in main_android.cpp
#include <android/log.h>
#include <string>

#define NOVA_LOG_TAG "NexusEngine"

namespace Nova {

enum class LogLevel { Trace=0, Info, Warning, Error, Fatal };

class Logger {
public:
    static Logger& Get() { static Logger inst; return inst; }
    void SetLevel(LogLevel l) { m_level=l; }
    LogLevel GetLevel() const { return m_level; }

    void Log(LogLevel level, const std::string& cat, const std::string& msg) {
        if (level < m_level) return;
        std::string out = "[" + cat + "] " + msg;
        switch(level) {
            case LogLevel::Trace:
            case LogLevel::Info:
                __android_log_print(ANDROID_LOG_INFO,  NOVA_LOG_TAG, "%s", out.c_str());
                break;
            case LogLevel::Warning:
                __android_log_print(ANDROID_LOG_WARN,  NOVA_LOG_TAG, "%s", out.c_str());
                break;
            case LogLevel::Error:
            case LogLevel::Fatal:
                __android_log_print(ANDROID_LOG_ERROR, NOVA_LOG_TAG, "%s", out.c_str());
                break;
        }
    }
    void Trace(const std::string& c, const std::string& m) { Log(LogLevel::Trace,   c, m); }
    void Info (const std::string& c, const std::string& m) { Log(LogLevel::Info,    c, m); }
    void Warn (const std::string& c, const std::string& m) { Log(LogLevel::Warning, c, m); }
    void Error(const std::string& c, const std::string& m) { Log(LogLevel::Error,   c, m); }
    void Fatal(const std::string& c, const std::string& m) { Log(LogLevel::Fatal,   c, m); }

private:
    LogLevel m_level = LogLevel::Info;
};

} // namespace Nova

#define NOVA_LOG_TRACE(cat, msg) ::Nova::Logger::Get().Trace(cat, msg)
#define NOVA_LOG_INFO(cat, msg)  ::Nova::Logger::Get().Info(cat, msg)
#define NOVA_LOG_WARN(cat, msg)  ::Nova::Logger::Get().Warn(cat, msg)
#define NOVA_LOG_ERROR(cat, msg) ::Nova::Logger::Get().Error(cat, msg)
#define NOVA_LOG_FATAL(cat, msg) ::Nova::Logger::Get().Fatal(cat, msg)
