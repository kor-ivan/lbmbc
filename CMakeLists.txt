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
    ${CMAKE_SOURCE_DIR}/version.h.in
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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS SerialBus)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(Qt6 REQUIRED COMPONENTS Concurrent)

add_executable(lbmbc
  main.cpp
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

# Указываем компилятору, где искать сгенерированный version.h
target_include_directories(lbmbc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG master # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_MakeAvailable(yaml-cpp)

target_link_libraries(lbmbc
    Qt${QT_VERSION_MAJOR}::Core
    Qt6::SerialBus
    yaml-cpp::yaml-cpp
    Qt6::Concurrent
)

# if(WIN32)
#     target_link_libraries(discover
#         wsock32
#         ws2_32
#         iphlpapi)
# endif()

include(GNUInstallDirs)
install(TARGETS lbmbc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
