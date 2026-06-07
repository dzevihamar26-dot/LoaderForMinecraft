#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include <string>
#include <functional>

namespace loader {

// Callback для прогресса распаковки
using ExtractProgressCallback = std::function<void(size_t extracted, size_t total)>;

class Extractor {
public:
    Extractor();
    ~Extractor();
    
    // Распаковать ZIP архив
    // Возвращает true при успехе, false при ошибке
    bool extractZip(const std::string& archivePath, const std::string& outputDir,
                    ExtractProgressCallback callback = nullptr);
    
    // Получить последнюю ошибку
    std::string getLastError() const;

private:
    std::string lastError_;
    
    // Реализация через miniz
    bool extractWithMiniz(const std::string& archivePath, const std::string& outputDir,
                          ExtractProgressCallback callback);
};

} // namespace loader

#endif // EXTRACTOR_HPP
