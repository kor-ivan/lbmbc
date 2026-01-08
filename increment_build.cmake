# increment_build.cmake
# Читаем текущий номер сборки из временного файла кэша
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/build_cache.txt")
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/build_cache.txt" BUILD_COUNT)
    math(EXPR BUILD_COUNT "${BUILD_COUNT} + 1")
else()
    set(BUILD_COUNT 1)
endif()

# Записываем обновленный номер обратно в кэш
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/build_cache.txt" "${BUILD_COUNT}")

# Передаем новый номер сборки в основной CMake файл
set(BUILD_NUMBER ${BUILD_COUNT} CACHE INTERNAL "Current build number")