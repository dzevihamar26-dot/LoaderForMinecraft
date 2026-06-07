#include "launcher.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace loader {

Launcher::Launcher() = default;

std::string Launcher::buildCommand(const std::string& javaPath, const std::string& jarPath,
                                    const std::string& ramAllocation) {
    // Формируем команду: java -Xmx{ram} -jar {jar}
    std::string command = "\"" + javaPath + "\" -Xmx" + ramAllocation + " -jar \"" + jarPath + "\"";
    
    return command;
}

LaunchResult Launcher::launch(const std::string& javaPath, const std::string& jarPath,
                               const std::string& ramAllocation, bool redirectOutput) {
    lastError_.clear();
    
    logInfo("Launching Minecraft client...");
    logInfo("Java path: " + javaPath);
    logInfo("JAR path: " + jarPath);
    logInfo("RAM allocation: " + ramAllocation);
    
    // Проверяем существование JAR файла
    if (!std::filesystem::exists(jarPath)) {
        lastError_ = "JAR file not found: " + jarPath;
        logError(lastError_);
        return {false, -1, lastError_};
    }
    
    // Проверяем существование Java
    if (!std::filesystem::exists(javaPath)) {
        lastError_ = "Java executable not found: " + javaPath;
        logError(lastError_);
        return {false, -1, lastError_};
    }
    
    // Строим команду
    std::string command = buildCommand(javaPath, jarPath, ramAllocation);
    logDebug("Launch command: " + command);
    
    // Запускаем процесс
    return executeProcess(command, redirectOutput);
}

LaunchResult Launcher::executeProcess(const std::string& command, bool redirectOutput) {
    logInfo("Starting process...");
    
#ifdef _WIN32
    // Windows реализация
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Копия команды, так как CreateProcess модифицирует строку
    std::string cmdCopy = command;
    
    DWORD creationFlags = CREATE_NEW_PROCESS_GROUP;
    
    if (!redirectOutput) {
        // Скрываем окно консоли для дочернего процесса
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    
    if (CreateProcessA(
            NULL,           // Имя приложения (NULL = из командной строки)
            (LPSTR)cmdCopy.c_str(),  // Командная строка
            NULL,           // Атрибуты процесса
            NULL,           // Атрибуты потока
            FALSE,          // Наследование дескрипторов
            creationFlags,  // Флаги создания
            NULL,           // Окружение (использовать текущее)
            NULL,           // Текущая директория
            &si,
            &pi
        )) {
        
        logSuccess("Minecraft process started successfully!");
        logInfo("Process ID: " + std::to_string(pi.dwProcessId));
        
        // Закрываем дескрипторы процесса и потока
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return {true, 0, "Process started successfully"};
    } else {
        DWORD errorCode = GetLastError();
        lastError_ = "Failed to start process. Windows error code: " + std::to_string(errorCode);
        logError(lastError_);
        return {false, -1, lastError_};
    }
    
#else
    // POSIX реализация (Linux/macOS)
    pid_t pid = fork();
    
    if (pid < 0) {
        lastError_ = "fork() failed";
        logError(lastError_);
        return {false, -1, lastError_};
    }
    
    if (pid == 0) {
        // Дочерний процесс
        
        if (!redirectOutput) {
            // Перенаправляем stdout/stderr в /dev/null
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
        }
        
        // Запускаем команду через execvp
        // Разбираем команду на аргументы
        std::vector<std::string> args;
        std::string arg;
        bool inQuotes = false;
        
        for (size_t i = 0; i < command.size(); ++i) {
            char c = command[i];
            
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ' ' && !inQuotes) {
                if (!arg.empty()) {
                    args.push_back(arg);
                    arg.clear();
                }
            } else {
                arg += c;
            }
        }
        if (!arg.empty()) {
            args.push_back(arg);
        }
        
        // Создаем массив C-строк
        std::vector<char*> argv;
        for (auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);
        
        execvp(argv[0], argv.data());
        
        // Если exec вернул управление, произошла ошибка
        _exit(127);
    } else {
        // Родительский процесс
        logSuccess("Minecraft process started successfully!");
        logInfo("Process PID: " + std::to_string(pid));
        
        // Ждем немного и проверяем, не завершился ли процесс сразу
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        
        if (result == 0) {
            // Процесс все еще работает
            return {true, 0, "Process started successfully"};
        } else if (result > 0) {
            // Процесс завершился
            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            lastError_ = "Process exited immediately with code: " + std::to_string(exitCode);
            logWarning(lastError_);
            return {false, exitCode, lastError_};
        } else {
            // Ошибка waitpid
            return {true, 0, "Process started (waitpid error, but process may be running)"};
        }
    }
#endif
}

std::string Launcher::getLastError() const {
    return lastError_;
}

} // namespace loader
