cmake_minimum_required(VERSION 3.22)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(dirserve)

FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.0
)
FetchContent_MakeAvailable(fmtlib)

set(SOURCES
    src/main.cpp
    src/utils/fd_guard.cpp
    src/threading/pool.cpp
    src/logging/severity.cpp
    src/logging/writer.cpp
    src/logging/logger.cpp
    src/http/transaction.cpp
    src/http/helpers.cpp
    src/http/subserver.cpp
    src/http/server.cpp
)

add_executable(dirserve ${SOURCES})

target_compile_options(dirserve PRIVATE -Wall -Wextra)

target_link_libraries(dirserve PRIVATE fmt::fmt)

target_include_directories(dirserve PRIVATE ${CMAKE_SOURCE_DIR}/src)
