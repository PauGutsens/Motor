#include "Logger.h"

Logger& Logger::instance() {
    static Logger g;
    return g;
}

void Logger::log(LogLevel level, const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    buf_.push_back(LogEntry{ level, text });
    if (buf_.size() > max_) buf_.pop_front();
}

void Logger::setMaxEntries(size_t n) {
    std::lock_guard<std::mutex> lock(mtx_);
    max_ = n;
    while (buf_.size() > max_) buf_.pop_front();
}

std::deque<LogEntry> Logger::snapshot() {
    std::lock_guard<std::mutex> lock(mtx_);
    return buf_;
}