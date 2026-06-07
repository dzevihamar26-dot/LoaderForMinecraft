#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <string>
#include <optional>

namespace loader {

struct LaunchResult {
    bool success;
    int exitCode;
    std::string message;
};

class Launcher {
public:
    Launcher();
    
    // Запустить Minecraft клиент
    // javaPath - путь к java executable
    // jarPath - путь к client.jar
    // ramAllocation - выделение памяти (например, "8G")
    // redirectOutput - перенаправлять вывод в консоль лоадера
    LaunchResult launch(const std::string& javaPath, const std::string& jarPath,
                        const std::string& ramAllocation = "8G",
                        bool redirectOutput = true);
    
    // Получить последнюю ошибку
    std::string getLastError() const;

private:
    std::string lastError_;
    
    // Сформировать команду запуска
    std::string buildCommand(const std::string& javaPath, const std::string& jarPath,
                             const std::string& ramAllocation);
    
    // Запустить процесс (кроссплатформенно)
    LaunchResult executeProcess(const std::string& command, bool redirectOutput);
};

} // namespace loader

#endif // LAUNCHER_HPP
