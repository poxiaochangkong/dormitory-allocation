#!/bin/bash
# Build script executed INSIDE Docker container
set -e

echo "=== Installing system packages ==="
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq \
  cmake make g++ pkgconf \
  libmysqlcppconn-dev \
  libmysqlclient-dev \
  libcpp-httplib-dev \
  nlohmann-json3-dev \
  libspdlog-dev \
  libssl-dev \
  curl

echo "=== Creating mysql/jdbc.h compatibility header ==="
mkdir -p /usr/include/mysql
cat > /usr/include/mysql/jdbc.h << 'EOF'
// Compatibility header: maps new mysql-connector-cpp 8.x <mysql/jdbc.h>
// to the older flat-header layout used by Debian/Ubuntu packages.
#pragma once

// Old-style JDBC headers (connector 1.1.x)
#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>
#include <cppconn/exception.h>
#include <cppconn/warning.h>
#include <cppconn/datatype.h>
#include <cppconn/sqlstring.h>
#include <cppconn/variant.h>
EOF

echo "=== Creating httplib CMake config ==="
mkdir -p /usr/lib/cmake/httplib
cat > /usr/lib/cmake/httplib/httplibConfig.cmake << 'EOF'
find_library(CPP_HTTPLIB_LIB cpp-httplib REQUIRED)
if(NOT TARGET httplib::httplib)
  add_library(httplib::httplib SHARED IMPORTED)
  set_target_properties(httplib::httplib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
    IMPORTED_LOCATION "${CPP_HTTPLIB_LIB}"
  )
  find_package(Threads REQUIRED)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(httplib::httplib INTERFACE
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()
EOF

echo "=== Fetching CLI11 header ==="
mkdir -p /usr/local/include/CLI
curl -sL -o /usr/local/include/CLI/CLI.hpp \
  https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp
echo "CLI11 downloaded: $(wc -c < /usr/local/include/CLI/CLI.hpp) bytes"

mkdir -p /usr/local/lib/cmake/CLI11
cat > /usr/local/lib/cmake/CLI11/CLI11Config.cmake << 'EOF'
set(CLI11_INCLUDE_DIR /usr/local/include/CLI)
if(NOT TARGET CLI11::CLI11)
  add_library(CLI11::CLI11 INTERFACE IMPORTED)
  set_target_properties(CLI11::CLI11 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CLI11_INCLUDE_DIR}"
  )
endif()
EOF

cat > /usr/local/lib/cmake/CLI11/CLI11ConfigVersion.cmake << 'EOF'
set(PACKAGE_VERSION "2.4.2")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_VERSION STREQUAL PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
EOF

echo "=== Configuring CMake ==="
mkdir -p build-linux
cd build-linux

cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUSE_SYSTEM_PACKAGES=ON

echo "=== Building ==="
cmake --build . -j$(nproc) 2>&1

echo "=== Build complete! ==="
ls -la dorm_alloc_*
file dorm_alloc_server
