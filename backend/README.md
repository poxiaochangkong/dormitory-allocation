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
