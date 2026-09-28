cmake_minimum_required(VERSION 3.16)

project(lbmbc
    VERSION 0.2.1
    LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. Поиск зависимостей ---
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network SerialBus Concurrent)


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
  lbmbc_global.h
  firmwarepackage.h firmwarepackage.cpp
  GenerateVersion.cmake
)
add_subdirectory(external/yaml-cpp)
target_link_libraries(lbmbc_lib PUBLIC yaml-cpp::yaml-cpp)

# Настройки оптимизации XZ
set(XZ_NLS OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZ OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_XZDEC OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_LZMADEC OFF CACHE BOOL "" FORCE)
set(XZ_TOOL_LZMAINFO OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

add_subdirectory(external/xz SYSTEM)
# Если таргеты созданы, мы принудительно переписываем их свойства COMPILE_OPTIONS.
# Это свойство Ninja и AUTOMOC читают в самую последнюю очередь перед запуском cc1plus.exe.
if(TARGET libgnu)
    set_property(TARGET libgnu PROPERTY COMPILE_OPTIONS "-w")
endif()
if(TARGET liblzma)
    set_property(TARGET liblzma PROPERTY COMPILE_OPTIONS "-w")
endif()
target_include_directories(lbmbc_lib SYSTEM PUBLIC external/xz/src/liblzma/api)
target_link_libraries(lbmbc_lib PRIVATE liblzma)


# Эта строка автоматически добавит #define LBMBC_LIBRARY при компиляции библиотеки
target_compile_definitions(lbmbc_lib PRIVATE LBMBC_LIBRARY)

set_target_properties(lbmbc_lib PROPERTIES
    OUTPUT_NAME "lbmbc"   # Имя файла будет lbmbc.dll
    PREFIX "lib"          # Явно добавляем lib в начало (будет liblbmbc.dll)
)

# Это сработает на Windows и будет проигнорировано на Linux
# set_target_properties(lbmbc_lib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

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

# === АВТО-ВЕРСИЯ: Создаем таргет для ежесборочной проверки Git и типа сборки ===
add_custom_target(lbmbc_version_check ALL
    COMMAND ${CMAKE_COMMAND}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/GenerateVersion.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${PROJECT_VERSION}"
        "${CMAKE_BUILD_TYPE}" # Передаем тип сборки шестым аргументом (ARGV6)
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    COMMENT "Checking Git repository status and build type for version.h..."
)

# --- 5. Создание ИСПОЛНЯЕМОГО ФАЙЛА (lbmbc) ---
# Собираем exe, если проект главный ИЛИ если родительский проект lbcfg разрешил сборку (не Debug)
if(PROJECT_IS_TOP_LEVEL OR BUILD_LBMBC_EXE)
    add_executable(lbmbc main.cpp)
    target_link_libraries(lbmbc PRIVATE lbmbc_lib)

    # Страховка для Multi-config (Visual Studio/Xcode): отключаем сборку lbmbc при выборе Debug
    set_target_properties(lbmbc PROPERTIES
        EXCLUDE_FROM_DEFAULT_BUILD_DEBUG TRUE
    )

    message(STATUS "lbmbc executable target ADDED")
else()
    message(STATUS "lbmbc executable target SKIPPED (Debug mode)")
endif()


# === АВТО-ВЕРСИЯ: Подключаем сгенерированные заголовочные файлы к lbmbc ===
target_include_directories(lbmbc_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(lbmbc_lib lbmbc_version_check)

# --- 6. Инсталляция ---
include(GNUInstallDirs)

# Инсталлируем общую библиотеку lbmbc_lib всегда, независимо от конфигурации
install(TARGETS lbmbc_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Инсталлируем исполняемый файл lbmbc, только если он был создан
if(TARGET lbmbc)
    install(TARGETS lbmbc
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        CONFIGURATIONS ${ALLOWED_LBMBC_CONFIG} # Динамически подставляем имя текущей не-Debug конфигурации
    )
endif()
