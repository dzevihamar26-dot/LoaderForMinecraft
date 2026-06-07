#include "logger.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define isatty(fd) _isatty(fd)
    #define fileno(fd) _fileno(fd)
#else
    #include <unistd.h>
#endif

namespace loader {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::isColorSupported() {
#ifdef _WIN32
    // Windows 10+ поддерживает ANSI escape коды
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return false;
    
    // Включаем обработку виртуальных терминалов
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(hOut, dwMode);
#else
    // Проверяем, является ли вывод терминалом
    return isatty(fileno(stdout));
#endif
}

std::string Logger::getLogLevelColor(LogLevel level) {
    if (!isColorSupported()) return "";
    
    switch (level) {
        case LogLevel::INFO:    return "\033[32m"; // Зеленый
        case LogLevel::WARNING: return "\033[33m"; // Желтый
        case LogLevel::ERROR:   return "\033[31m"; // Красный
        case LogLevel::SUCCESS: return "\033[36m"; // Голубой
        case LogLevel::DEBUG:   return "\033[90m"; // Серый
        default:                return "";
    }
}

std::string Logger::getLogLevelPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "[INFO]";
        case LogLevel::WARNING: return "[WARN]";
        case LogLevel::ERROR:   return "[ERROR]";
        case LogLevel::SUCCESS: return "[OK]";
        case LogLevel::DEBUG:   return "[DEBUG]";
        default:                return "[LOG]";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::string color = getLogLevelColor(level);
    std::string prefix = getLogLevelPrefix(level);
    std::string reset = isColorSupported() ? "\033[0m" : "";
    
    std::cout << color << prefix << reset << " " << message << std::endl;
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::success(const std::string& message) {
    log(LogLevel::SUCCESS, message);
}

void Logger::debug(const std::string& message) {
#ifdef DEBUG_BUILD
    log(LogLevel::DEBUG, message);
#else
    (void)message; // Подавить предупреждение о неиспользуемой переменной
#endif
}

void Logger::showProgress(size_t current, size_t total, const std::string& label) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (total == 0) return;
    
    int percentage = static_cast<int>((current * 100) / total);
    int barWidth = 40;
    int filled = static_cast<int>((current * barWidth) / total);
    
    std::string reset = isColorSupported() ? "\033[0m" : "";
    std::string green = isColorSupported() ? "\033[32m" : "";
    
    std::cout << "\r" << label << ": [";
    std::cout << green;
    
    for (int i = 0; i < barWidth; ++i) {
        if (i < filled) {
            std::cout << "=";
        } else if (i == filled && current < total) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    
    std::cout << reset << "] " << percentage << "% (" 
              << current << "/" << total << ")";
    std::cout.flush();
    
    if (current >= total) {
        std::cout << std::endl;
    }
}

void Logger::clearLine() {
    std::cout << "\r\033[K" << std::flush;
}

// Глобальные функции
void logInfo(const std::string& message) {
    Logger::getInstance().info(message);
}

void logWarning(const std::string& message) {
    Logger::getInstance().warning(message);
}

void logError(const std::string& message) {
    Logger::getInstance().error(message);
}

void logSuccess(const std::string& message) {
    Logger::getInstance().success(message);
}

void logDebug(const std::string& message) {
    Logger::getInstance().debug(message);
}

void showProgress(size_t current, size_t total, const std::string& label) {
    Logger::getInstance().showProgress(current, total, label);
}

} // namespace loader
