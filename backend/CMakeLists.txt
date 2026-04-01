cmake_minimum_required(VERSION 3.14)

# Dormitory Allocation Backend (C++)
#
# IMPORTANT COMPATIBILITY NOTE:
# - C++ standard is set to C++14 to be friendly to more compilers/environments.
# - We provide CMakePresets.json for convenience (requires CMake >= 3.19 to *use* presets).

project(dorm_alloc_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------
# Dependencies (provided by vcpkg manifest mode)
# ----------------------------
# These find_package calls rely on vcpkg toolchain integration.
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)

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
