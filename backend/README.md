# Backend (C++): Dormitory Allocation

This folder contains the C++ backend skeleton.

## Goals of this skeleton

1. **Clear architecture / separation of concerns** (easy for others to read and extend).
2. **Frontend-independent debugging** via a CLI executable.
3. **MySQL bootstrap**: every startup verifies the database exists and creates it if absent.

## Targets

- `dorm_alloc_core` (library)
  - Domain/Application/Algorithm/Infrastructure skeleton.
- `dorm_alloc_server` (executable)
  - REST API server based on `cpp-httplib`.
- `dorm_alloc_cli` (executable)
  - Command line tool for backend debugging without frontend.

## Build

This project uses:

- CMake (minimum 3.14; presets provided for convenience)
- vcpkg in **manifest mode**

Typical local build (with presets):

```bash
cmake --preset default
cmake --build --preset default
```

## Configuration

Executables read a JSON config file (see `config/config.example.json`).
It contains MySQL connection details and the database name.

## MySQL connector mode (important)

- The backend now uses **mysql-connector-cpp Classic API (JDBC/cppconn)**.
- This means DB connection uses standard MySQL protocol/port (**3306** by default),
  not X DevAPI/X Protocol (commonly 33060).
- In vcpkg, `mysql-connector-cpp[jdbc]` on Windows requires a static triplet,
  so the preset sets `VCPKG_TARGET_TRIPLET=x64-windows-static`.

## Run (启动后端)

### 前置条件

1. **MySQL 服务已启动**，且 `config/config.example.json` 中的连接信息正确：
   ```json
   {
       "mysql": {
           "host": "127.0.0.1",
           "port": 3306,
           "user": "root",
           "password": "root",
           "database": "dorm_alloc"
       }
   }
   ```
2. **已构建项目**（参见上方 Build 章节）

### 启动命令

```bash
# 进入 backend 目录（可执行文件依赖同目录下的 config/）
cd backend

# Windows (MSVC Debug build)
./build/Debug/dorm_alloc_server.exe

# Windows (MSVC Release build)
./build/Release/dorm_alloc_server.exe

# Linux / macOS
./build/dorm_alloc_server
```

启动成功后会看到：
```
[server] database ready: dorm_alloc
[server] listening on 0.0.0.0:8080
```

### 验证

```bash
curl http://localhost:8080/health
# 应返回: OK
```
