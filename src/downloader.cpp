#include "downloader.hpp"
#include "logger.hpp"

#include <curl/curl.h>
#include <fstream>
#include <cstring>

namespace loader {

// Структура для передачи данных в callback
struct DownloadData {
    std::ofstream* file;
    ProgressCallback callback;
    size_t totalSize;
    size_t downloadedSize;
};

// Callback для записи данных
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    DownloadData* data = static_cast<DownloadData*>(userp);
    
    if (data->file && data->file->is_open()) {
        data->file->write(static_cast<char*>(contents), realsize);
        data->downloadedSize += realsize;
        
        if (data->callback) {
            data->callback(data->downloadedSize, data->totalSize);
        }
    }
    
    return realsize;
}

// Callback для получения размера файла
static int headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    DownloadData* data = static_cast<DownloadData*>(userdata);
    
    // Ищем заголовок Content-Length
    std::string header(buffer, size * nitems);
    const std::string prefix = "Content-Length:";
    
    size_t pos = header.find(prefix);
    if (pos != std::string::npos) {
        std::string value = header.substr(pos + prefix.length());
        // Trim пробелов
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(0, 1);
        }
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) {
            value.pop_back();
        }
        
        if (!value.empty()) {
            try {
                data->totalSize = std::stoull(value);
            } catch (...) {
                // Игнорируем ошибки парсинга
            }
        }
    }
    
    return size * nitems;
}

Downloader::Downloader() : cancelled_(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Downloader::~Downloader() {
    curl_global_cleanup();
}

bool Downloader::download(const std::string& url, const std::string& outputPath,
                          ProgressCallback callback) {
    lastError_.clear();
    cancelled_ = false;
    
    return downloadWithCurl(url, outputPath, callback);
}

bool Downloader::downloadWithCurl(const std::string& url, const std::string& outputPath,
                                   ProgressCallback callback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        lastError_ = "Failed to initialize libcurl";
        logError(lastError_);
        return false;
    }
    
    // Открываем файл для записи
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        lastError_ = "Failed to open output file: " + outputPath;
        logError(lastError_);
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Подготавливаем данные для callback
    DownloadData data;
    data.file = &file;
    data.callback = callback;
    data.totalSize = 0;
    data.downloadedSize = 0;
    
    // Настраиваем libcurl
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);
    
    // Таймауты
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L); // Без ограничения на общее время
    
    // User agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MinecraftLoader/1.0");
    
    // SSL опции
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    logInfo("Starting download from: " + url);
    logInfo("Output file: " + outputPath);
    
    // Выполняем запрос
    CURLcode res = curl_easy_perform(curl);
    
    file.close();
    
    if (res != CURLE_OK) {
        lastError_ = "Download failed: " + std::string(curl_easy_strerror(res));
        logError(lastError_);
        
        // Удаляем частично скачанный файл
        std::remove(outputPath.c_str());
        
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Проверяем HTTP код ответа
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    
    if (responseCode != 200) {
        lastError_ = "HTTP error: " + std::to_string(responseCode);
        logError(lastError_);
        std::remove(outputPath.c_str());
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_cleanup(curl);
    
    logSuccess("Download completed successfully");
    return true;
}

std::string Downloader::getLastError() const {
    return lastError_;
}

void Downloader::cancel() {
    cancelled_ = true;
}

} // namespace loader
