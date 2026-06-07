#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <filesystem>
#include <optional>

namespace loader {
namespace utils {

// Раскрыть тильду до домашней директории
std::string expandTilde(const std::string& path);

// Получить путь к домашней директории
std::string getHomeDirectory();

// Проверить, существует ли директория, если нет - создать
bool ensureDirectoryExists(const std::string& path);

// Получить текущую рабочую директорию
std::string getCurrentWorkingDirectory();

// Объединить пути кроссплатформенно
std::string joinPath(const std::string& base, const std::string& path);

// Получить расширение файла
std::string getFileExtension(const std::string& filename);

// Получить имя файла без расширения
std::string getFileNameWithoutExtension(const std::string& filename);

// Проверить, является ли файл исполняемым JAR
bool isJarFile(const std::string& path);

// Получить размер файла
std::optional<std::uintmax_t> getFileSize(const std::string& path);

// Удалить файл
bool deleteFile(const std::string& path);

// Проверить, является ли строка валидным URL
bool isValidUrl(const std::string& url);

// Экранировать путь для Windows
std::string escapePath(const std::string& path);

} // namespace utils
} // namespace loader

#endif // UTILS_HPP
