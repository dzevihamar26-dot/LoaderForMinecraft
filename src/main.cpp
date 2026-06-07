#include <iostream>
#include <string>
#include <limits>

#include "logger.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "downloader.hpp"
#include "extractor.hpp"
#include "java_finder.hpp"
#include "launcher.hpp"

using namespace loader;

// Очистка ввода
void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Запрос с валидацией ответа Y/n
bool askYesNo(const std::string& question, bool defaultYes = false) {
    std::cout << question << " ";
    std::string answer;
    std::getline(std::cin, answer);
    
    if (answer.empty()) {
        return defaultYes;
    }
    
    char c = std::tolower(answer[0]);
    return c == 'y' || c == 'д'; // Поддержка русской раскладки
}

// Запрос строки с возможностью значения по умолчанию
std::string askString(const std::string& prompt, const std::string& defaultValue = "") {
    std::cout << prompt;
    if (!defaultValue.empty()) {
        std::cout << " [" << defaultValue << "]";
    }
    std::cout << ": ";
    
    std::string answer;
    std::getline(std::cin, answer);
    
    if (answer.empty() && !defaultValue.empty()) {
        return defaultValue;
    }
    
    return answer;
}

// Выбор версии Java
std::string selectJavaVersion() {
    std::cout << "\n=== Выбор версии Java ===" << std::endl;
    std::cout << "Доступные версии: 8, 11, 17, 21" << std::endl;
    
    std::vector<int> validVersions = {8, 11, 17, 21};
    
    while (true) {
        std::cout << "Введите номер версии: ";
        std::string input;
        std::getline(std::cin, input);
        
        try {
            int version = std::stoi(input);
            
            for (int v : validVersions) {
                if (version == v) {
                    return std::to_string(version);
                }
            }
            
            std::cout << "Неверная версия. Пожалуйста, выберите из списка: 8, 11, 17, 21" << std::endl;
        } catch (...) {
            std::cout << "Некорректный ввод. Введите число." << std::endl;
        }
    }
}

// Получить путь по умолчанию
std::string getDefaultInstallPath() {
#ifdef _WIN32
    return "C:\\mc_client";
#else
    return utils::joinPath(utils::getHomeDirectory(), "mc_client");
#endif
}

int main(int argc, char* argv[]) {
    // Приветствие
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "   Minecraft Client Loader v" << PROJECT_VERSION << std::endl;
    std::cout << "   Cross-platform installer & launcher" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n";
    
    logInfo("Welcome to Minecraft Client Loader!");
    
    // Инициализация менеджера конфигурации
    ConfigManager configManager;
    
    // Попытка загрузить существующие настройки
    auto settings = configManager.loadSettings();
    
    if (settings.has_value()) {
        std::cout << "\n--- Сохранённые настройки найдены ---" << std::endl;
        std::cout << "Java версия: " << settings->javaVersion << std::endl;
        std::cout << "Путь установки: " << settings->installDirectory << std::endl;
        std::cout << "Выделение памяти: " << settings->maxRamAllocation << std::endl;
        
        if (askYesNo("Использовать сохранённые настройки?", true)) {
            logInfo("Используем сохранённые настройки");
        } else {
            logInfo("Запускаем полную настройку");
            settings = std::nullopt;
        }
    }
    
    // Полная настройка если нужно
    if (!settings.has_value()) {
        std::cout << "\n=== Настройка лоадера ===" << std::endl;
        
        Settings newSettings;
        
        // Выбор версии Java
        newSettings.javaVersion = selectJavaVersion();
        logInfo("Выбрана версия Java: " + newSettings.javaVersion);
        
        // Выбор директории установки
        std::string defaultPath = getDefaultInstallPath();
        std::string installPath = askString("Путь для установки клиента", defaultPath);
        
        // Раскрываем тильду если есть
        installPath = utils::expandTilde(installPath);
        
        // Проверяем/создаем директорию
        if (!utils::ensureDirectoryExists(installPath)) {
            logError("Не удалось создать директорию: " + installPath);
            return 1;
        }
        
        newSettings.installDirectory = installPath;
        logInfo("Директория установки: " + installPath);
        
        // Выделение памяти
        std::string ram = askString("Выделение памяти (например, 8G)", "8G");
        newSettings.maxRamAllocation = ram;
        
        // Путь к JAR файлу (по умолчанию client.jar)
        newSettings.lastUsedJarPath = "client.jar";
        
        settings = newSettings;
        
        // Сохраняем настройки
        if (configManager.saveSettings(*settings)) {
            logSuccess("Настройки сохранены в settings.json");
        } else {
            logWarning("Не удалось сохранить настройки");
        }
    }
    
    // Загружаем .env файл
    EnvConfig envConfig = EnvConfig::getDefault();
    auto loadedEnv = configManager.loadEnvFile();
    if (loadedEnv.has_value()) {
        envConfig = *loadedEnv;
        logDebug(".env файл загружен");
    }
    
    // Проверяем URL архива
    if (envConfig.clientArchiveUrl.empty()) {
        logError("CLIENT_ARCHIVE_URL не указан в .env файле!");
        logError("Пожалуйста, отредактируйте .env файл и укажите URL для скачивания клиента.");
        return 1;
    }
    
    // Путь к JAR файлу
    std::string jarPath = utils::joinPath(settings->installDirectory, settings->lastUsedJarPath);
    
    // Проверка наличия клиента
    bool reinstall = false;
    if (std::filesystem::exists(jarPath)) {
        std::cout << "\nКлиент уже установлен: " << jarPath << std::endl;
        if (!askYesNo("Переустановить?", false)) {
            logInfo("Переустановка отменена, переходим к запуску");
            reinstall = false;
        } else {
            reinstall = true;
        }
    }
    
    // Скачивание и распаковка если нужно
    if (!std::filesystem::exists(jarPath) || reinstall) {
        std::cout << "\n=== Скачивание клиента ===" << std::endl;
        
        std::string archivePath = utils::joinPath(settings->installDirectory, "client_archive.zip");
        
        Downloader downloader;
        
        bool downloadSuccess = downloader.download(
            envConfig.clientArchiveUrl,
            archivePath,
            [](size_t downloaded, size_t total) {
                showProgress(downloaded, total, "Downloading");
            }
        );
        
        if (!downloadSuccess) {
            logError("Ошибка загрузки: " + downloader.getLastError());
            return 1;
        }
        
        // Распаковка
        std::cout << "\n=== Распаковка файлов ===" << std::endl;
        
        Extractor extractor;
        
        bool extractSuccess = extractor.extractZip(
            archivePath,
            settings->installDirectory,
            [](size_t extracted, size_t total) {
                showProgress(extracted, total, "Extracting");
            }
        );
        
        if (!extractSuccess) {
            logError("Ошибка распаковки: " + extractor.getLastError());
            utils::deleteFile(archivePath);
            return 1;
        }
        
        // Удаляем архив после распаковки
        utils::deleteFile(archivePath);
        
        // Проверяем, что JAR файл существует после распаковки
        if (!std::filesystem::exists(jarPath)) {
            logWarning("Файл " + settings->lastUsedJarPath + " не найден после распаковки");
            logInfo("Поиск JAR файлов в директории установки...");
            
            // Пытаемся найти любой JAR файл
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(settings->installDirectory)) {
                    if (entry.is_regular_file() && utils::isJarFile(entry.path().string())) {
                        jarPath = entry.path().string();
                        settings->lastUsedJarPath = entry.path().filename().string();
                        logInfo("Найден JAR файл: " + jarPath);
                        
                        // Обновляем настройки
                        configManager.saveSettings(*settings);
                        break;
                    }
                }
            } catch (...) {
                // Игнорируем ошибки
            }
        }
    }
    
    // Поиск и проверка Java
    std::cout << "\n=== Проверка Java ===" << std::endl;
    
    int targetVersion = std::stoi(settings->javaVersion);
    
    JavaFinder javaFinder;
    auto javaInfo = javaFinder.findJavaByVersion(targetVersion);
    
    if (!javaInfo.has_value()) {
        logError("Java версии " + settings->javaVersion + " не найдена в системе!");
        logError("Пожалуйста, установите требуемую версию Java.");
        logError("Ссылки для загрузки:");
        logError("  - Oracle JDK: https://www.oracle.com/java/technologies/downloads/");
        logError("  - OpenJDK: https://adoptium.net/");
        return 1;
    }
    
    logSuccess("Найдена Java " + javaInfo->version);
    logInfo("Путь: " + javaInfo->path);
    
    // Запуск клиента
    std::cout << "\n=== Запуск клиента ===" << std::endl;
    
    Launcher launcher;
    auto result = launcher.launch(
        javaInfo->path,
        jarPath,
        settings->maxRamAllocation,
        true // Перенаправляем вывод в консоль
    );
    
    if (result.success) {
        logSuccess("Minecraft запущен успешно!");
        std::cout << "\nДля остановки Minecraft нажмите Ctrl+C в окне игры." << std::endl;
    } else {
        logError("Ошибка запуска: " + result.message);
        return 1;
    }
    
    return 0;
}
