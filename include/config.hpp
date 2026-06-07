#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <optional>
#include <map>

namespace loader {

struct Settings {
    std::string installDirectory;
    std::string javaVersion;
    std::string lastUsedJarPath;
    std::string maxRamAllocation;
    
    // Значения по умолчанию
    static Settings getDefault();
};

struct EnvConfig {
    std::string clientArchiveUrl;
    std::string clientJarFilename;
    
    // Значения по умолчанию
    static EnvConfig getDefault();
};

class ConfigManager {
public:
    ConfigManager(const std::string& workingDir = "");
    
    // Загрузка настроек из settings.json
    std::optional<Settings> loadSettings();
    
    // Сохранение настроек в settings.json
    bool saveSettings(const Settings& settings);
    
    // Загрузка .env файла
    std::optional<EnvConfig> loadEnvFile();
    
    // Парсинг строки .env
    static std::map<std::string, std::string> parseEnvContent(const std::string& content);
    
    // Получить путь к settings.json
    std::string getSettingsPath() const;
    
    // Получить путь к .env
    std::string getEnvPath() const;

private:
    std::string workingDir_;
    
    // Чтение файла в строку
    std::optional<std::string> readFile(const std::string& path);
    
    // Запись строки в файл
    bool writeFile(const std::string& path, const std::string& content);
};

} // namespace loader

#endif // CONFIG_HPP
