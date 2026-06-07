#include "extractor.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <miniz.h>
#include <vector>
#include <cstring>

namespace loader {

Extractor::Extractor() = default;
Extractor::~Extractor() = default;

bool Extractor::extractZip(const std::string& archivePath, const std::string& outputDir,
                           ExtractProgressCallback callback) {
    lastError_.clear();
    
    return extractWithMiniz(archivePath, outputDir, callback);
}

bool Extractor::extractWithMiniz(const std::string& archivePath, const std::string& outputDir,
                                  ExtractProgressCallback callback) {
    logInfo("Extracting archive: " + archivePath);
    logInfo("Output directory: " + outputDir);
    
    // Открываем ZIP архив
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));
    
    if (!mz_zip_reader_init_file(&zipArchive, archivePath.c_str(), 0)) {
        lastError_ = "Failed to open ZIP archive";
        logError(lastError_);
        return false;
    }
    
    // Получаем количество файлов
    mz_uint numEntries = mz_zip_reader_get_num_files(&zipArchive);
    logInfo("Files in archive: " + std::to_string(numEntries));
    
    if (numEntries == 0) {
        lastError_ = "Archive is empty";
        logError(lastError_);
        mz_zip_reader_end(&zipArchive);
        return false;
    }
    
    // Обеспечиваем существование выходной директории
    if (!utils::ensureDirectoryExists(outputDir)) {
        lastError_ = "Failed to create output directory";
        logError(lastError_);
        mz_zip_reader_end(&zipArchive);
        return false;
    }
    
    size_t extractedCount = 0;
    
    // Проходим по всем файлам в архиве
    for (mz_uint i = 0; i < numEntries; i++) {
        mz_zip_archive_file_stat fileStat;
        
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            logWarning("Failed to get file stat for entry " + std::to_string(i));
            continue;
        }
        
        std::string filename(fileStat.m_filename);
        
        // Пропускаем записи без имени
        if (filename.empty()) {
            continue;
        }
        
        // Пропускаем директории (они будут созданы автоматически)
        bool isDir = mz_zip_reader_is_file_a_directory(&zipArchive, i);
        
        if (isDir) {
            // Создаем директорию
            std::string dirPath = utils::joinPath(outputDir, filename);
            utils::ensureDirectoryExists(dirPath);
            continue;
        }
        
        // Создаем родительские директории для файла
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            std::string parentDir = utils::joinPath(outputDir, filename.substr(0, lastSlash));
            utils::ensureDirectoryExists(parentDir);
        }
        
        // Полный путь для извлечения
        std::string extractPath = utils::joinPath(outputDir, filename);
        
        // Открываем файл для записи
        FILE* outFile = fopen(extractPath.c_str(), "wb");
        if (!outFile) {
            logWarning("Failed to create file: " + extractPath);
            continue;
        }
        
        // Извлекаем файл
        void* buffer = mz_zip_reader_extract_file_to_heap(&zipArchive, filename.c_str(), nullptr, 0);
        if (buffer) {
            fwrite(buffer, 1, fileStat.m_uncomp_size, outFile);
            fclose(outFile);
            mz_free(buffer);
            
            extractedCount++;
            
            // Вызываем callback прогресса
            if (callback) {
                callback(extractedCount, numEntries);
            }
        } else {
            fclose(outFile);
            logWarning("Failed to extract file: " + filename);
        }
    }
    
    mz_zip_reader_end(&zipArchive);
    
    logSuccess("Extraction completed. Files extracted: " + std::to_string(extractedCount));
    return true;
}

std::string Extractor::getLastError() const {
    return lastError_;
}

} // namespace loader
