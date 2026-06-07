#include "config.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace loader {

Settings Settings::getDefault() {
    Settings settings;
#ifdef _WIN32
    settings.installDirectory = "C:\\mc_client";
#else
    settings.installDirectory = utils::joinPath(utils::getHomeDirectory(), "mc_client");
#endif
    settings.javaVersion = "17";
    settings.lastUsedJarPath = "client.jar";
    settings.maxRamAllocation = "8G";
    return settings;
}

EnvConfig EnvConfig::getDefault() {
    EnvConfig config;
    config.clientArchiveUrl = "";
    config.clientJarFilename = "client.jar";
    return config;
}

ConfigManager::ConfigManager(const std::string& workingDir) 
    : workingDir_(workingDir.empty() ? utils::getCurrentWorkingDirectory() : workingDir) {
}

std::string ConfigManager::getSettingsPath() const {
    return utils::joinPath(workingDir_, "settings.json");
}

std::string ConfigManager::getEnvPath() const {
    return utils::joinPath(workingDir_, ".env");
}

std::optional<std::string> ConfigManager::readFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (...) {
        return std::nullopt;
    }
}

bool ConfigManager::writeFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        file << content;
        return file.good();
    } catch (...) {
        return false;
    }
}

std::optional<Settings> ConfigManager::loadSettings() {
    std::string path = getSettingsPath();
    
    auto content = readFile(path);
    if (!content.has_value()) {
        logDebug("Settings file not found: " + path);
        return std::nullopt;
    }
    
    try {
        json j = json::parse(content.value());
        
        Settings settings;
        
        // Читаем поля с проверкой существования
        if (j.contains("InstallDirectory") && j["InstallDirectory"].is_string()) {
            settings.installDirectory = j["InstallDirectory"].get<std::string>();
        } else {
            settings.installDirectory = Settings::getDefault().installDirectory;
        }
        
        if (j.contains("JavaVersion") && j["JavaVersion"].is_string()) {
            settings.javaVersion = j["JavaVersion"].get<std::string>();
        } else {
            settings.javaVersion = Settings::getDefault().javaVersion;
        }
        
        if (j.contains("LastUsedJarPath") && j["LastUsedJarPath"].is_string()) {
            settings.lastUsedJarPath = j["LastUsedJarPath"].get<std::string>();
        } else {
            settings.lastUsedJarPath = Settings::getDefault().lastUsedJarPath;
        }
        
        if (j.contains("MaxRamAllocation") && j["MaxRamAllocation"].is_string()) {
            settings.maxRamAllocation = j["MaxRamAllocation"].get<std::string>();
        } else {
            settings.maxRamAllocation = Settings::getDefault().maxRamAllocation;
        }
        
        logDebug("Settings loaded successfully");
        return settings;
    } catch (const json::exception& e) {
        logWarning("Failed to parse settings.json: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool ConfigManager::saveSettings(const Settings& settings) {
    std::string path = getSettingsPath();
    
    try {
        json j;
        j["InstallDirectory"] = settings.installDirectory;
        j["JavaVersion"] = settings.javaVersion;
        j["LastUsedJarPath"] = settings.lastUsedJarPath;
        j["MaxRamAllocation"] = settings.maxRamAllocation;
        
        std::string content = j.dump(4); // Красивый вывод с отступами
        return writeFile(path, content);
    } catch (const json::exception& e) {
        logError("Failed to serialize settings: " + std::string(e.what()));
        return false;
    }
}

std::map<std::string, std::string> ConfigManager::parseEnvContent(const std::string& content) {
    std::map<std::string, std::string> result;
    
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Ищем знак '='
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim пробелов
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
            key.pop_back();
        }
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(0, 1);
        }
        
        // Удаляем кавычки если есть
        if (value.size() >= 2 && 
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        
        if (!key.empty()) {
            result[key] = value;
        }
    }
    
    return result;
}

std::optional<EnvConfig> ConfigManager::loadEnvFile() {
    std::string path = getEnvPath();
    
    auto content = readFile(path);
    if (!content.has_value()) {
        logDebug(".env file not found: " + path);
        return std::nullopt;
    }
    
    try {
        auto envMap = parseEnvContent(content.value());
        
        EnvConfig config;
        
        auto it = envMap.find("CLIENT_ARCHIVE_URL");
        if (it != envMap.end()) {
            config.clientArchiveUrl = it->second;
        }
        
        it = envMap.find("CLIENT_JAR_FILENAME");
        if (it != envMap.end()) {
            config.clientJarFilename = it->second;
        }
        
        logDebug(".env file loaded successfully");
        return config;
    } catch (...) {
        logWarning("Failed to parse .env file");
        return std::nullopt;
    }
}

} // namespace loader
