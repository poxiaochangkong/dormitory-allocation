#!/bin/bash
# Build the backend inside a Docker container using system packages.
# This avoids the slow vcpkg bootstrap + compilation of boost etc.
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build-linux"

echo "[build] Starting Docker-based build..."

# Clean previous build
rm -rf "$BUILD_DIR"

docker run --rm \
  -v "$PROJECT_DIR":/workspace \
  -w /workspace \
  ubuntu:24.04 bash -c "

set -e

echo '[build] Installing system packages...'
apt-get update -qq
apt-get install -y -qq \
  cmake make g++ pkgconf \
  libmysqlcppconn-dev \
  libcpp-httplib-dev \
  nlohmann-json3-dev \
  libspdlog-dev \
  libssl-dev

# CLI11 is not in apt — download the single-header version
echo '[build] Fetching CLI11 header...'
mkdir -p /usr/local/include/CLI
curl -sL -o /usr/local/include/CLI/CLI.hpp \
  https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp

# Create a tiny CMake config for CLI11 so find_package works
mkdir -p /usr/local/lib/cmake/CLI11
cat > /usr/local/lib/cmake/CLI11/CLI11Config.cmake << 'CMEOF'
set(CLI11_INCLUDE_DIR /usr/local/include/CLI)
if(NOT TARGET CLI11::CLI11)
  add_library(CLI11::CLI11 INTERFACE IMPORTED)
  set_target_properties(CLI11::CLI11 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES \"\${CLI11_INCLUDE_DIR}\"
  )
endif()
CMEOF

cat > /usr/local/lib/cmake/CLI11/CLI11ConfigVersion.cmake << 'CMEOF'
set(PACKAGE_VERSION \"2.4.2\")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_VERSION STREQUAL PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
CMEOF

echo '[build] Configuring CMake...'
mkdir -p build-linux
cd build-linux

# Create a modified CMakeLists override for system packages
cat > /tmp/FindMySQLCppConn.cmake << 'FINDEOL'
# Helper to find MySQL Connector C++ from system packages (Ubuntu/Debian)
# Provides: mysqlcppconn::mysqlcppconn and mysqlclient::mysqlclient
set(MYSQLCPPCONN_FOUND TRUE)

# Find the JDBC header
find_path(MYSQLCPPCONN_JDBC_INCLUDE_DIR mysql/jdbc.h
  PATHS /usr/include/mysql-cppconn-8 /usr/include
  PATH_SUFFIXES mysql-cppconn-8 mysql
)
if(NOT MYSQLCPPCONN_JDBC_INCLUDE_DIR)
  message(FATAL_ERROR \"mysql/jdbc.h not found\")
endif()

# Find the library
find_library(MYSQLCPPCONN_LIBRARY mysqlcppconn)
if(NOT MYSQLCPPCONN_LIBRARY)
  message(FATAL_ERROR \"mysqlcppconn library not found\")
endif()

# Find MySQL client library (for libmysql)
find_library(MYSQLCLIENT_LIBRARY mysqlclient)
if(NOT MYSQLCLIENT_LIBRARY)
  message(FATAL_ERROR \"mysqlclient library not found\")
endif()

if(NOT TARGET mysqlcppconn::mysqlcppconn)
  add_library(mysqlcppconn::mysqlcppconn SHARED IMPORTED)
  set_target_properties(mysqlcppconn::mysqlcppconn PROPERTIES
    IMPORTED_LOCATION \"\${MYSQLCPPCONN_LIBRARY}\"
    INTERFACE_INCLUDE_DIRECTORIES \"\${MYSQLCPPCONN_JDBC_INCLUDE_DIR}\"
  )
endif()

if(NOT TARGET mysqlclient::mysqlclient)
  add_library(mysqlclient::mysqlclient SHARED IMPORTED)
  set_target_properties(mysqlclient::mysqlclient PROPERTIES
    IMPORTED_LOCATION \"\${MYSQLCLIENT_LIBRARY}\"
  )
endif()
FINDEOL

cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUSE_SYSTEM_PACKAGES=ON \
  2>&1

echo '[build] Compiling...'
cmake --build . -j\$(nproc) 2>&1

echo '[build] Done!'
"

echo "[build] Build complete. Output in: $BUILD_DIR"
ls -la "$BUILD_DIR"/dorm_alloc_* 2>/dev/null || echo "Check build-linux directory for binaries"
