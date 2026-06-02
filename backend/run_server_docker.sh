#!/bin/bash
# Run the backend server in Docker with proper networking
set -e

docker run --rm \
  --name dorm-backend \
  -p 8080:8080 \
  -v /home/myprecious/dormitory-allocation/backend:/workspace \
  -w /workspace \
  ubuntu:24.04 bash -c '
set -e
export DEBIAN_FRONTEND=noninteractive
echo "[server] Installing runtime dependencies..."
apt-get update -qq
apt-get install -y -qq \
  libmysqlcppconn7t64 \
  libcpp-httplib0.14t64 \
  libspdlog1.12 \
  libfmt9 \
  libmysqlclient21

echo "[server] Starting dorm_alloc_server..."
./build-linux/dorm_alloc_server --config config/config.docker.json
'
