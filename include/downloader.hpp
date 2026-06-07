#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <string>
#include <functional>

namespace loader {

// Callback для прогресса загрузки
using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;

class Downloader {
public:
    Downloader();
    ~Downloader();
    
    // Скачать файл по URL
    // Возвращает true при успехе, false при ошибке
    bool download(const std::string& url, const std::string& outputPath, 
                  ProgressCallback callback = nullptr);
    
    // Получить последнюю ошибку
    std::string getLastError() const;
    
    // Отменить загрузку
    void cancel();

private:
    std::string lastError_;
    bool cancelled_;
    
    // Внутренняя реализация через libcurl
    bool downloadWithCurl(const std::string& url, const std::string& outputPath,
                          ProgressCallback callback);
};

} // namespace loader

#endif // DOWNLOADER_HPP
