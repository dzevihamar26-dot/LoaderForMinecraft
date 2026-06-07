cmake_minimum_required(VERSION 3.15)
project(MinecraftLoader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции сборки
option(BUILD_STATIC "Build static executable" ON)
option(USE_SYSTEM_CURL "Use system curl instead of vcpkg" OFF)
option(USE_SYSTEM_NLOHMANN_JSON "Use system nlohmann/json instead of vcpkg" OFF)

# Статическая линковка для Windows
if(WIN32 AND BUILD_STATIC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Поиск или загрузка зависимостей
include(FetchContent)

# nlohmann/json - используем системную библиотеку если доступна
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# libcurl - используем системную библиотеку
find_package(CURL REQUIRED)

# miniz для работы с ZIP архивами (header-only библиотека, загружаем локально)
FetchContent_Declare(
    miniz
    URL https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h
    DOWNLOAD_NO_EXTRACT ON
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/miniz-src
)
FetchContent_Populate(miniz)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/downloader.cpp
    src/extractor.cpp
    src/java_finder.cpp
    src/launcher.cpp
    src/logger.cpp
    src/utils.cpp
)

set(HEADERS
    include/config.hpp
    include/downloader.hpp
    include/extractor.hpp
    include/java_finder.hpp
    include/launcher.hpp
    include/logger.hpp
    include/utils.hpp
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${miniz_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
    miniz
)

# Платформенно-зависимые библиотеки
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
endif()

# Компиляционные определения
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Debug>:DEBUG_BUILD>
)

# Установка
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES .env.example DESTINATION . RENAME .env OPTIONAL)
