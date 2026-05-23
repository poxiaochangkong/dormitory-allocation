# Backend (C++): Dormitory Allocation

This folder contains the C++ backend for the dormitory allocation system.

## Goals

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
- `dorm_alloc_test` (executable)
  - Integration tests against a real MySQL database.

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

## 默认管理员账户

系统首次启动时会自动创建一个管理员账户：

| 字段     | 值          |
|----------|-------------|
| 学号     | `admin`     |
| 密码     | `admin123`  |
| user_id  | `admin_001` |

- 密码使用 **SHA-256 + 随机 salt** 哈希存储，不以明文保存。
- 如果数据库中已存在 admin 用户，不会重复创建。
- 旧版本（明文密码）会在启动时自动迁移为哈希格式。

## 认证机制

- 所有用户密码使用 `CryptoUtil::HashPassword(password, salt)` 存储，底层为 SHA-256。
- 每个用户有独立的随机 salt（32 位十六进制字符串）。
- 登录成功后生成 token（`tk_` 前缀 + 32 位随机十六进制），存储在 `user.token` 字段。
- 管理员登录通过 `AdminService::Login`（仅允许 `role='admin'` 的用户）。
- 学生登录通过 `StudentService::Login`（允许所有用户）。

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

## API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| POST | `/api/student/login` | 学生登录 |
| POST | `/api/student/register` | 学生注册 |
| GET | `/api/student/info` | 获取学生信息 |
| GET | `/api/student/questionnaire/status` | 问卷状态 |
| POST | `/api/student/questionnaire/submit` | 提交问卷 |
| POST | `/api/student/scene/submit` | 提交沉浸式场景数据 |
| GET | `/api/student/match-result` | 查看分配结果 |
| POST | `/api/admin/login` | 管理员登录 |
| GET | `/api/admin/users` | 用户列表 |
| POST | `/api/admin/import` | 批量导入学生 |
| DELETE | `/api/admin/user` | 删除用户 |
| POST | `/api/admin/transfer-admin` | 转移管理员权限 |
| POST | `/api/admin/task/create` | 创建分配任务 |
| POST | `/api/admin/task/run` | 执行分配算法 |
| GET | `/api/admin/task/result` | 查看任务结果 |
| POST | `/api/admin/task/adjust` | 手动调整（交换） |
| GET | `/api/admin/task/export` | 导出 CSV |
| GET | `/api/admin/tasks` | 任务列表 |
| GET | `/api/admin/dormitories` | 宿舍列表 |
| POST | `/api/admin/dormitory/add` | 添加宿舍 |
| POST | `/api/admin/rule/save` | 保存分配规则 |

## 集成测试

项目包含 29 个集成测试用例，覆盖认证、用户管理、分配算法等全部功能：

```bash
# 进入 backend 目录
cd backend

# 运行集成测试（需要 MySQL 服务已启动）
./build/Debug/dorm_alloc_test.exe --config config/config.example.json
```

测试内容包括：
1. 数据库连接
2. SHA-256 哈希（空字符串、"abc"）
3. Salt 生成唯一性
4. 密码哈希与验证
5. 测试数据清理
6. 批量导入学生（admin 过滤）
7. 学生登录（正确/错误密码）
8. 管理员登录
9. 学生注册（重复注册拒绝）
10. 用户列表、删除、管理员权限转移
11. 问卷提交（12 名学生）
12. 宿舍管理（6 间房间）
13. 分配任务创建与执行
14. 分配结果验证
15. 手动调整（交换学生）
16. CSV 导出

预期输出：
```
=====================================
  ALL 29 TESTS PASSED
=====================================