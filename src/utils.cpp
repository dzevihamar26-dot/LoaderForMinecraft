#include "utils.hpp"
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace loader {
namespace utils {

std::string getHomeDirectory() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
    if (home != nullptr) {
        return std::string(home);
    }
    // Fallback
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (drive != nullptr && path != nullptr) {
        return std::string(drive) + std::string(path);
    }
    return "C:\\";
#else
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        return std::string(home);
    }
    // Fallback через passwd
    struct passwd* pw = getpwuid(getuid());
    if (pw != nullptr) {
        return std::string(pw->pw_dir);
    }
    return "/";
#endif
}

std::string expandTilde(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    
    if (path.size() == 1 || path[1] == '/' 
#ifdef _WIN32
        || path[1] == '\\'
#endif
    ) {
        return getHomeDirectory() + path.substr(1);
    }
    
    return path;
}

bool ensureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(path);
        
        if (std::filesystem::exists(fsPath)) {
            return std::filesystem::is_directory(fsPath);
        }
        
        return std::filesystem::create_directories(fsPath);
    } catch (const std::filesystem::filesystem_error& e) {
        return false;
    }
}

std::string getCurrentWorkingDirectory() {
    try {
        return std::filesystem::current_path().string();
    } catch (...) {
        return ".";
    }
}

std::string joinPath(const std::string& base, const std::string& path) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(base) / std::filesystem::path(path);
        return fsPath.string();
    } catch (...) {
#ifdef _WIN32
        return base + "\\" + path;
#else
        return base + "/" + path;
#endif
    }
}

std::string getFileExtension(const std::string& filename) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(filename);
        return fsPath.extension().string();
    } catch (...) {
        size_t pos = filename.rfind('.');
        if (pos != std::string::npos && pos < filename.size() - 1) {
            return filename.substr(pos);
        }
        return "";
    }
}

std::string getFileNameWithoutExtension(const std::string& filename) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(filename);
        return fsPath.stem().string();
    } catch (...) {
        size_t lastSlash = filename.find_last_of("/\\");
        size_t lastDot = filename.rfind('.');
        
        std::string name = (lastSlash != std::string::npos) ? 
                           filename.substr(lastSlash + 1) : filename;
        
        if (lastDot != std::string::npos && lastDot > (lastSlash == std::string::npos ? 0 : lastSlash)) {
            return name.substr(0, lastDot - (lastSlash != std::string::npos ? lastSlash + 1 : 0));
        }
        return name;
    }
}

bool isJarFile(const std::string& path) {
    std::string ext = getFileExtension(path);
    return ext == ".jar" || ext == ".JAR";
}

std::optional<std::uintmax_t> getFileSize(const std::string& path) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(path);
        if (std::filesystem::exists(fsPath) && std::filesystem::is_regular_file(fsPath)) {
            return std::filesystem::file_size(fsPath);
        }
    } catch (...) {
        // Игнорируем ошибки
    }
    return std::nullopt;
}

bool deleteFile(const std::string& path) {
    try {
        std::filesystem::path fsPath = std::filesystem::path(path);
        if (std::filesystem::exists(fsPath) && std::filesystem::is_regular_file(fsPath)) {
            return std::filesystem::remove(fsPath);
        }
    } catch (...) {
        // Игнорируем ошибки
    }
    return false;
}

bool isValidUrl(const std::string& url) {
    if (url.empty()) return false;
    
    // Простая проверка на наличие протокола
    return url.find("http://") == 0 || url.find("https://") == 0;
}

std::string escapePath(const std::string& path) {
#ifdef _WIN32
    // Экранирование обратных слешей для Windows
    std::string result;
    for (char c : path) {
        if (c == '\\') {
            result += "\\\\";
        } else {
            result += c;
        }
    }
    return result;
#else
    return path;
#endif
}

} // namespace utils
} // namespace loader
