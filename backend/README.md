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
