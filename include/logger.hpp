#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <mutex>

namespace loader {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    SUCCESS,
    DEBUG
};

class Logger {
public:
    static Logger& getInstance();
    
    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void success(const std::string& message);
    void debug(const std::string& message);
    
    // Прогресс бар
    void showProgress(size_t current, size_t total, const std::string& label = "Progress");
    void clearLine();

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string getLogLevelColor(LogLevel level);
    std::string getLogLevelPrefix(LogLevel level);
    bool isColorSupported();
    
    std::mutex logMutex_;
};

// Глобальные функции для удобства
void logInfo(const std::string& message);
void logWarning(const std::string& message);
void logError(const std::string& message);
void logSuccess(const std::string& message);
void logDebug(const std::string& message);
void showProgress(size_t current, size_t total, const std::string& label = "Progress");

} // namespace loader

#endif // LOGGER_HPP
