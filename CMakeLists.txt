cmake_minimum_required(VERSION 3.16)

project(lbmbc
    VERSION 0.1.1
    LANGUAGES CXX)

# 1. Инкрементируем номер сборки при каждом конфигурировании CMake
include(increment_build.cmake)

# 2. Извлекаем короткий хеш последнего коммита Git
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(GIT_COMMIT_HASH "n/a") # Если Git не найден или нет репозитория
endif()

# Устанавливаем переменную CMake, которая будет использоваться в configure_file
set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE INTERNAL "Current git commit hash")

# 3. Генерируем итоговый version.h из шаблона version.h.in
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (UNIX AND NOT APPLE)
    # set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_PREFIX_PATH
        "/home/ivan/git/lbmbc/build/install/"
        ${CMAKE_PREFIX_PATH}
    )
endif()

# --- 3. Поиск зависимостей ---
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network SerialBus Concurrent)

# Внешняя зависимость yaml-cpp
include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG master # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_MakeAvailable(yaml-cpp)

# --- 4. Создание БИБЛИОТЕКИ (lbmbc_lib) ---
# Теперь вся логика здесь
add_library(lbmbc_lib SHARED
  mbclient.h mbclient.cpp
  lbclient.h lbclient.cpp
  lbmodbusclient.h lbmodbusclient.cpp
  lbyaml.h lbyaml.cpp
  qtyaml.cpp
  icmp6scanner.h icmp6scanner.cpp
  discover.h discover.cpp
  lbconsole.h lbconsole.cpp
  lbprocess.h lbprocess.cpp
  version.h.in
)

set_target_properties(lbmbc_lib PROPERTIES
    OUTPUT_NAME "lbmbc"   # Имя файла будет lbmbc.dll
    PREFIX "lib"          # Явно добавляем lib в начало (будет liblbmbc.dll)
)

# Это сработает на Windows и будет проигнорировано на Linux
set_target_properties(lbmbc_lib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Пробрасываем пути к заголовкам (включая сгенерированный version.h)
target_include_directories(lbmbc_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)


# Линкуем зависимости к БИБЛИОТЕКЕ как PUBLIC.
# Это заставит lbcfg видеть Qt и yaml-cpp автоматически.
target_link_libraries(lbmbc_lib PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::SerialBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    yaml-cpp::yaml-cpp
)

# # --- 5. Создание ИСПОЛНЯЕМОГО ФАЙЛА (lbmbc) ---
# add_executable(lbmbc main.cpp)

# # Он просто использует нашу библиотеку
# target_link_libraries(lbmbc PRIVATE lbmbc_lib)

# # --- 6. Инсталляция ---
# include(GNUInstallDirs)
# install(TARGETS lbmbc lbmbc_lib
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )



# --- 5. Создание ИСПОЛНЯЕМОГО ФАЙЛА (lbmbc) ---
# Собираем exe только если проект запущен как главный, а не как подмодуль
if(PROJECT_IS_TOP_LEVEL)
    add_executable(lbmbc main.cpp)
    target_link_libraries(lbmbc PRIVATE lbmbc_lib)

    message(STATUS "lbmbc is TOP LEVEL project: building executable")
else()
    message(STATUS "lbmbc is a dependency: skipping executable")
endif()

# --- 6. Инсталляция ---
include(GNUInstallDirs)

# Инсталлируем библиотеку всегда
install(TARGETS lbmbc_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Инсталлируем exe только если он был создан
if(PROJECT_IS_TOP_LEVEL)
    install(TARGETS lbmbc
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
