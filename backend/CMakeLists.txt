#此文件定义了CMake构建系统的配置，用于构建宿舍分配系统的后端部分。
#它指定了项目名称、C++标准、依赖项以及构建目标（核心库、服务器可执行文件和CLI可执行文件）。
#该配置使用vcpkg管理依赖项，并确保在Windows上使用静态运行时库以避免DLL依赖问题。

cmake_minimum_required(VERSION 3.14)

# Keep MSVC runtime consistent with vcpkg static triplet dependencies
# (e.g. mysql-connector-cpp[jdbc] on x64-windows-static uses /MT,/MTd).
# NOTE: CMP0091 must be set BEFORE the first project()/enable_language().
cmake_policy(SET CMP0091 NEW)

# Dormitory Allocation Backend (C++)
#
# IMPORTANT COMPATIBILITY NOTE:
# - C++ standard is set to C++14 to be friendly to more compilers/environments.
# - We provide CMakePresets.json for convenience (requires CMake >= 3.19 to *use* presets).

project(dorm_alloc_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ----------------------------
# Dependencies (provided by vcpkg manifest mode)
# ----------------------------
# These find_package calls rely on vcpkg toolchain integration.
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(unofficial-libmysql CONFIG REQUIRED)

# ----------------------------
# Core library: domain/application/algorithm/infrastructure
# ----------------------------
add_library(dorm_alloc_core)

target_include_directories(dorm_alloc_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dorm_alloc_core
  PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Use Classic API (JDBC) target if available.
# This keeps DB access on standard MySQL protocol/port (default 3306),
# instead of X DevAPI/X Protocol (typically 33060).
if(TARGET unofficial::mysql-connector-cpp::connector-jdbc)
  target_link_libraries(dorm_alloc_core
    PUBLIC
      unofficial::mysql-connector-cpp::connector-jdbc
      unofficial::libmysql::libmysql
  )
else()
  message(FATAL_ERROR
    "mysql-connector-cpp jdbc target not found. "
    "Please ensure vcpkg installs mysql-connector-cpp[jdbc] with a supported triplet "
    "(e.g. x64-windows-static)."
  )
endif()

target_sources(dorm_alloc_core
  PRIVATE
    src/infrastructure/config/AppConfig.cpp
    src/infrastructure/db/MySqlClient.cpp
    src/infrastructure/db/DbBootstrapper.cpp
    src/algorithm/MatchEngine.cpp
    src/service/StudentService.cpp
    src/service/AdminService.cpp
)

# ----------------------------
# Server executable (REST API)
# ----------------------------
add_executable(dorm_alloc_server
  src/server/main.cpp
)

target_link_libraries(dorm_alloc_server
  PRIVATE
    dorm_alloc_core
    httplib::httplib
)

# ----------------------------
# CLI executable (debug without frontend)
# ----------------------------
add_executable(dorm_alloc_cli
  src/cli/main.cpp
)

target_link_libraries(dorm_alloc_cli
  PRIVATE
    dorm_alloc_core
    CLI11::CLI11
)

# ----------------------------
# Integration test executable
# ----------------------------
add_executable(dorm_alloc_test
  tests/integration_test.cpp
)

target_link_libraries(dorm_alloc_test
  PRIVATE
    dorm_alloc_core
)
