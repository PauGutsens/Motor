#pragma once
#include <deque>
#include <string>
#include <mutex>

enum class LogLevel { Info, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string text;
};

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& text);
    void setMaxEntries(size_t n);
    std::deque<LogEntry> snapshot(); // 拷贝当前消息用于 ImGui 渲染

private:
    Logger() = default;
    std::mutex mtx_;
    std::deque<LogEntry> buf_;
    size_t max_ = 1000;
};

// 便捷宏
#define LOG_INFO(msg)   Logger::instance().log(LogLevel::Info,   std::string("[INFO] ") + (msg))
#define LOG_WARN(msg)   Logger::instance().log(LogLevel::Warning,std::string("[WARN] ") + (msg))
#define LOG_ERROR(msg)  Logger::instance().log(LogLevel::Error,  std::string("[ERROR] ")+ (msg))
