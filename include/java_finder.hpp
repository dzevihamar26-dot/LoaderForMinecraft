#ifndef JAVA_FINDER_HPP
#define JAVA_FINDER_HPP

#include <string>
#include <optional>
#include <vector>

namespace loader {

struct JavaInfo {
    std::string path;           // Путь к исполняемому файлу java
    std::string version;        // Версия Java (например, "17.0.1")
    int majorVersion;           // Основная версия (например, 17)
    std::string javaHome;       // JAVA_HOME директория
};

class JavaFinder {
public:
    JavaFinder();
    
    // Найти Java в системе
    // Возвращает информацию о найденной Java или nullopt если не найдена
    std::optional<JavaInfo> findJava();
    
    // Проверить конкретный путь к Java
    std::optional<JavaInfo> checkJavaPath(const std::string& javaPath);
    
    // Найти Java по версии (8, 11, 17, 21)
    std::optional<JavaInfo> findJavaByVersion(int targetVersion);
    
    // Получить список всех найденных Java установок
    std::vector<JavaInfo> findAllJavaInstallations();
    
    // Получить последнюю ошибку
    std::string getLastError() const;

private:
    std::string lastError_;
    
    // Парсинг вывода java -version
    std::optional<JavaInfo> parseJavaVersion(const std::string& output, const std::string& path);
    
    // Поиск в стандартных путях
    std::vector<std::string> getStandardJavaPaths();
    
    // Выполнить команду и получить вывод
    std::string executeCommand(const std::string& command);
    
    // Проверить переменные окружения
    std::optional<JavaInfo> checkEnvVariables();
};

} // namespace loader

#endif // JAVA_FINDER_HPP
