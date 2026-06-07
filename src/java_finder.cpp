#include "java_finder.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <cstdlib>
#include <array>
#include <regex>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <unistd.h>
#endif

namespace loader {

JavaFinder::JavaFinder() = default;

std::string JavaFinder::executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
#ifdef _WIN32
    // Windows версия
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        return "";
    }

    SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "cmd.exe /c " + command;
    
    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(g_hChildStd_OUT_Wr);
        g_hChildStd_OUT_Wr = NULL;

        DWORD dwRead;
        while (ReadFile(g_hChildStd_OUT_Rd, buffer.data(), buffer.size() - 1, &dwRead, NULL) && dwRead > 0) {
            buffer[dwRead] = '\0';
            result += buffer.data();
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(g_hChildStd_OUT_Rd);
    } else {
        if (g_hChildStd_OUT_Wr) CloseHandle(g_hChildStd_OUT_Wr);
        if (g_hChildStd_OUT_Rd) CloseHandle(g_hChildStd_OUT_Rd);
        return "";
    }
#else
    // POSIX версия
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    pclose(pipe);
#endif
    
    return result;
}

std::optional<JavaInfo> JavaFinder::parseJavaVersion(const std::string& output, const std::string& path) {
    // Парсим вывод java -version
    // Примеры:
    // java version "1.8.0_301"
    // java version "11.0.12" 2021-07-20 LTS
    // openjdk version "17.0.1" 2021-10-19
    
    std::regex versionRegex(R"((?:java|openjdk)\s+version\s+"([^"]+)")", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(output, match, versionRegex) && match.size() > 1) {
        std::string versionStr = match[1].str();
        
        // Извлекаем основную версию
        int majorVersion = 0;
        
        // Для старых версий типа 1.8.0_xxx
        if (versionStr.find("1.") == 0) {
            majorVersion = std::stoi(versionStr.substr(2, 1));
        } else {
            // Для новых версий типа 11.0.12, 17.0.1
            size_t dotPos = versionStr.find('.');
            if (dotPos != std::string::npos) {
                try {
                    majorVersion = std::stoi(versionStr.substr(0, dotPos));
                } catch (...) {
                    majorVersion = 0;
                }
            }
        }
        
        JavaInfo info;
        info.path = path;
        info.version = versionStr;
        info.majorVersion = majorVersion;
        
        // Пытаемся определить JAVA_HOME
        std::filesystem::path javaPath(path);
        if (javaPath.has_parent_path()) {
            auto binDir = javaPath.parent_path();
            if (binDir.has_parent_path()) {
                info.javaHome = binDir.parent_path().string();
            }
        }
        
        return info;
    }
    
    return std::nullopt;
}

std::vector<std::string> JavaFinder::getStandardJavaPaths() {
    std::vector<std::string> paths;
    
#ifdef _WIN32
    // Windows стандартные пути
    const char* programFiles = std::getenv("ProgramFiles");
    const char* programFilesX86 = std::getenv("ProgramFiles(x86)");
    
    if (programFiles) {
        paths.push_back(std::string(programFiles) + "\\Java");
    }
    if (programFilesX86) {
        paths.push_back(std::string(programFilesX86) + "\\Java");
    }
    
    // Добавляем конкретные пути
    paths.push_back("C:\\Program Files\\Java");
    paths.push_back("C:\\Program Files (x86)\\Java");
#else
    // Linux/macOS стандартные пути
    paths.push_back("/usr/lib/jvm");
    paths.push_back("/usr/java");
    paths.push_back("/Library/Java/JavaVirtualMachines");
    
#ifdef __APPLE__
    // macOS специфичный путь
    std::string home = utils::getHomeDirectory();
    paths.push_back(home + "/Library/Java/JavaVirtualMachines");
#endif
#endif
    
    return paths;
}

std::optional<JavaInfo> JavaFinder::checkEnvVariables() {
    // Проверяем JAVA_HOME
    const char* javaHome = std::getenv("JAVA_HOME");
    if (javaHome != nullptr && strlen(javaHome) > 0) {
        std::string javaPath = utils::joinPath(javaHome, "bin");
#ifdef _WIN32
        javaPath = utils::joinPath(javaPath, "java.exe");
#else
        javaPath = utils::joinPath(javaPath, "java");
#endif
        
        auto info = checkJavaPath(javaPath);
        if (info.has_value()) {
            info->javaHome = javaHome;
            return info;
        }
    }
    
    // Проверяем PATH через команду which/where
#ifdef _WIN32
    std::string output = executeCommand("where java 2>nul");
#else
    std::string output = executeCommand("which java 2>/dev/null");
#endif
    
    if (!output.empty()) {
        // Берем первую строку (может быть несколько путей)
        size_t newlinePos = output.find('\n');
        std::string javaPath = (newlinePos != std::string::npos) ? 
                               output.substr(0, newlinePos) : output;
        
        // Trim пробелов и символов новой строки
        while (!javaPath.empty() && (javaPath.back() == '\n' || javaPath.back() == '\r' || 
               javaPath.back() == ' ' || javaPath.back() == '\t')) {
            javaPath.pop_back();
        }
        
        if (!javaPath.empty()) {
            auto info = checkJavaPath(javaPath);
            if (info.has_value()) {
                return info;
            }
        }
    }
    
    return std::nullopt;
}

std::optional<JavaInfo> JavaFinder::checkJavaPath(const std::string& javaPath) {
    // Проверяем существование файла
    if (!std::filesystem::exists(javaPath)) {
        return std::nullopt;
    }
    
    // Выполняем java -version
    std::string command = "\"" + javaPath + "\" -version 2>&1";
    std::string output = executeCommand(command);
    
    if (output.empty()) {
        return std::nullopt;
    }
    
    return parseJavaVersion(output, javaPath);
}

std::optional<JavaInfo> JavaFinder::findJava() {
    lastError_.clear();
    
    // Сначала проверяем переменные окружения
    auto envJava = checkEnvVariables();
    if (envJava.has_value()) {
        logInfo("Found Java via environment: " + envJava->path);
        logInfo("Java version: " + envJava->version);
        return envJava;
    }
    
    // Ищем в стандартных путях
    auto standardPaths = getStandardJavaPaths();
    
    for (const auto& basePath : standardPaths) {
        if (!std::filesystem::exists(basePath)) {
            continue;
        }
        
        // Рекурсивно ищем java executable
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
#ifdef _WIN32
                    if (filename == "java.exe") {
#else
                    if (filename == "java") {
#endif
                        auto info = checkJavaPath(entry.path().string());
                        if (info.has_value()) {
                            logInfo("Found Java at: " + info->path);
                            logInfo("Java version: " + info->version);
                            return info;
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Игнорируем ошибки доступа
            continue;
        }
    }
    
    lastError_ = "Java not found in system";
    logError(lastError_);
    return std::nullopt;
}

std::optional<JavaInfo> JavaFinder::findJavaByVersion(int targetVersion) {
    lastError_.clear();
    
    // Сначала проверяем переменные окружения
    auto envJava = checkEnvVariables();
    if (envJava.has_value() && envJava->majorVersion == targetVersion) {
        logInfo("Found Java " + std::to_string(targetVersion) + " via environment");
        return envJava;
    }
    
    // Ищем в стандартных путях
    auto standardPaths = getStandardJavaPaths();
    
    for (const auto& basePath : standardPaths) {
        if (!std::filesystem::exists(basePath)) {
            continue;
        }
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
#ifdef _WIN32
                    if (filename == "java.exe") {
#else
                    if (filename == "java") {
#endif
                        auto info = checkJavaPath(entry.path().string());
                        if (info.has_value() && info->majorVersion == targetVersion) {
                            logInfo("Found Java " + std::to_string(targetVersion) + " at: " + info->path);
                            return info;
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
    }
    
    lastError_ = "Java version " + std::to_string(targetVersion) + " not found";
    logError(lastError_);
    return std::nullopt;
}

std::vector<JavaInfo> JavaFinder::findAllJavaInstallations() {
    std::vector<JavaInfo> installations;
    
    // Собираем все найденные Java
    auto standardPaths = getStandardJavaPaths();
    
    for (const auto& basePath : standardPaths) {
        if (!std::filesystem::exists(basePath)) {
            continue;
        }
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
#ifdef _WIN32
                    if (filename == "java.exe") {
#else
                    if (filename == "java") {
#endif
                        auto info = checkJavaPath(entry.path().string());
                        if (info.has_value()) {
                            // Проверяем, нет ли уже такой установки
                            bool exists = false;
                            for (const auto& existing : installations) {
                                if (existing.path == info->path) {
                                    exists = true;
                                    break;
                                }
                            }
                            if (!exists) {
                                installations.push_back(*info);
                            }
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
    }
    
    return installations;
}

std::string JavaFinder::getLastError() const {
    return lastError_;
}

} // namespace loader
