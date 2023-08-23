cmake_minimum_required(VERSION 3.12)

project(VSRO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_executable(VSRO main.cpp)
target_include_directories(VSRO PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/include")

if (UNIX)
    find_package(fmt)
    target_link_libraries(VSRO PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/lib/libraylib.so" fmt::fmt)
endif (UNIX)

if (WIN32)
    target_include_directories(VSRO PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../fmt/include")
    target_link_libraries(VSRO PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/lib/raylib.dll" "${CMAKE_CURRENT_SOURCE_DIR}/../fmt/bin/libfmt.dll")
endif (WIN32)
