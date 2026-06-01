# Bug Report

> 生成时间：2026-06-01
> 范围：前后端对接问题、静态代码分析发现的 bug
> 说明：仅列出未修复的 bug

---

## 🔴 严重缺陷

### C2. SQL 注入 —— 所有 SQL 均使用字符串拼接

**文件**: `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`, `server/main.cpp`

**描述**: 所有 SQL 查询通过字符串拼接用户输入构建。虽然有 `Escape()` 函数处理单引号，但不能防御全部注入向量（反引号、null bytes、多字节字符攻击）。

**影响**: 数据库可能被攻破。

**当前状态**: ⚠️ 部分缓解 — `Escape()` 覆盖了最常见的单引号向量。完整防护需改用 prepared statements + 参数绑定，改动量较大。

---

### C3. Token 使用 `std::rand()` 生成 —— 非密码学安全

**文件**: `StudentService.cpp`, `AdminService.cpp`

**描述**: 认证 Token 由 `std::rand()` + 毫秒时间戳种子生成。`std::rand()` 不是密码学安全的 PRNG，Token 值可预测。

**影响**: Token 预测 → 会话劫持。

---

### C4. `AdjustResult` 只交换 dorm_id，不更新 roommate_ids

**文件**: `AdminService.cpp` — `AdjustResult()`

**描述**: 管理员调整分配结果时只更新了 `dorm_id`，未重新计算受影响房间的 `roommate_ids`，导致数据不一致。

**影响**: 调整后室友列表数据错误。

---

### C5. `ImportStudents` 总是生成新 user_id

**文件**: `AdminService.cpp` — `ImportStudents()`

**描述**: 每次导入生成新 `user_id`（`GenId("u_")`）。若 `student_no` 已存在，`ON DUPLICATE KEY UPDATE` 会更新旧行，但 `count++` 仍然递增，导入数量统计误导。

**影响**: 给管理员展示错误的导入计数。

---

## 🟠 高优先级

### H2. `GreedyAssign` 中 `dorm_idx` 变量多余

**文件**: `MatchEngine.cpp` — `GreedyAssign()`

**描述**: `for (const auto &dorm : dorms)` 已遍历寝室，但内部还维护 `dorm_idx` 做冗余边界检查，是死代码。

**影响**: 无功能影响，但代码可读性差。

---

### H3. `MatchEngine::Match()` 声明了但未实现

**文件**: `MatchEngine.h` / `MatchEngine.cpp`

**描述**: 头文件声明了 `Match()` 方法，实际实现是静态方法 `ExecuteAllocation()`。调用 `Match()` 会链接错误。

**影响**: 编译通过但链接时出错。

---

### H6. `SaveAllocationRule` 保存 JSON 时转义不完整

**文件**: `AdminService.cpp` — `SaveAllocationRule()`

**描述**: `rule_json` 经 `Escape()` 只转义了单引号，JSON 中的反斜杠等 SQL 特殊字符未被处理。

**影响**: 保存复杂规则时可能 SQL 报错或注入。

---

## 🟡 中等优先级

### M1. `GenerateToken()` 调用 `std::srand()` 影响全局随机状态

**文件**: `StudentService.cpp`, `AdminService.cpp`

**描述**: 每次调用 `std::srand()` 重设全局随机数种子。若两个登录请求间隔很短，可能得到相同种子 → 相同 Token。

**影响**: Token 碰撞 → 会话劫持。

---

### M2. `GenId()` 用毫秒 + rand 生成 ID

**文件**: `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`

**描述**: 时间戳毫秒 + `rand() % 10000`。高并发下 ID 会碰撞。

**影响**: 主键重复错误或静默数据覆盖。

---

### M3. `AdminDashboard.vue` 调用了未定义的 `refreshTaskList()`

**文件**: `AdminDashboard.vue`

**描述**: 模板中引用了 `refreshTaskList()`，但组件 methods 中未定义此函数。

**影响**: 点击刷新按钮时 JS 运行时错误，任务列表无法刷新。

---

### M4. 问卷 `nightOwls` 字段未在匹配算法中使用

**文件**: `QuestionnaireView.vue` / `StudentService.cpp`

**描述**: 问卷表单包含 `nightOwls` 字段，但后端 `SubmitQuestionnaire` 仅存入 `raw_answers` JSON，匹配算法未使用该数据。

**影响**: 用户偏好数据被收集但不参与匹配。

---

### M6. `GetMatchResult` 返回虚假子分数

**文件**: `StudentService.cpp` — `GetMatchResult()`

**描述**: 返回的 `hygieneConsistencyScore` 和 `scheduleOverlapScore` 由公式 `sim_score * 0.9 + 0.05`、`sim_score * 0.85 + 0.1` 计算，并非算法的真实度量。

**影响**: 前端雷达图展示误导性数据。

---

### M7. `ListTasks` 和 `ListDormitories` 无认证

**文件**: `server/main.cpp`

**描述**: `GET /api/admin/tasks` 和 `GET /api/admin/dormitories` 没有认证检查。

**影响**: 任何人可以未登录查看所有任务和寝室。

---

### M8. `ExportResult` CSV 未转义字段中的逗号

**文件**: `AdminService.cpp` — `ExportResult()`

**描述**: 若 college、major 等字段含逗号，CSV 格式会被破坏。

**影响**: 导出的 CSV 在 Excel 等工具中解析错误。

---

### Bug 13. 无分配规则配置 UI

**文件**: `AdminDashboard.vue`

**问题**: 后端已实现 `POST /api/admin/allocation/rule/save`，前端 `api/index.js` 已定义 `saveAllocationRule()`，但缺少规则配置 UI 页面。

**影响**: 管理员无法通过界面配置分配规则。

---

### Bug 15. AdminAdjust 无法手动调整学生分配

**文件**: `AdminAdjust.vue`

**问题**: 后端已实现交换接口，前端页面有拖拽 UI 但实际无法使用，没有完整的操作流程。

**影响**: 管理员无法手动干预分配结果。

---

### Bug 16. 后端多个端点无前端对接

| 后端端点 | 功能 | 前端状态 |
|---------|------|---------|
| `GET /api/student/questionnaire/template` | 获取问卷模板 | 无 API 函数，无调用 |
| `GET /api/student/questionnaire/status/:userId` | 查询问卷状态 | 无 API 函数，无调用 |
| `GET /api/student/info/:userId` | 获取学生信息 | 无 API 函数，无调用 |
| `GET /api/admin/allocation/rules` | 获取分配规则 | 无 API 函数，无调用 |
| `GET /api/admin/dashboard/stats` | 仪表盘统计 | 无 API 函数，无调用 |

---

### Bug 17. StudentResult 空状态提示不够友好

**文件**: `StudentResult.vue`

**问题**: 未分配时只显示"暂无分配结果"，缺少引导用户完成问卷的提示。

**影响**: 用户体验欠佳。

---

## 🔵 低优先级

### L1. 注册无密码强度校验

**文件**: `StudentService.cpp` — `Register()`

**描述**: 任何密码均接受，仅通过 `password.empty()` 拦空白串。

---

### L2. 前端 API baseURL 硬编码为 `localhost:8080`

**文件**: `frontend-matching/src/api/index.js`

**描述**: axios baseURL 硬编码，应使用环境变量配置。

---

### L3. 登录端点无速率限制

**文件**: `server/main.cpp`

**描述**: `/api/student/login` 和 `/api/admin/login` 无暴力破解防护。

---

### L4. 单锁 `db_mutex` 串行化所有请求

**文件**: `server/main.cpp`

**描述**: 单个 mutex 守护所有数据库访问，高负载下成为瓶颈。

---

### L5. Token 永不过期

**文件**: `StudentService.cpp`, `AdminService.cpp`

**描述**: Token 无过期机制，除非重新登录（生成新 Token 隐式作废旧 Token）。

---

## 前后端接口对比

| # | 端点 | 前端发送 | 后端接收 | 状态 |
|---|------|---------|---------|------|
| 1 | Admin Login | `{ studentNo, password }` | `{ studentNo, password }` | ✅ 匹配 |
| 2 | Questionnaire Submit | 字母编码(A/B/C/D) | 字符串/数值 | ✅ 已修复(加映射) |
| 3 | Scene Submit | Scene data JSON | JSON + userId | ✅ 已修复 |
| 4 | Match Result | GET + Bearer token | Token auth | ✅ 已修复 |
| 5 | Admin Task Create | `{ taskName, ... }` | `{ taskName, ... }` | ✅ 匹配 |
| 6 | Admin Import | JSON array | JSON array | ✅ 匹配 |

---

## 修复建议（按优先级）

1. **C3**: 使用 CSPRNG 生成 Token（`std::random_device` + `std::mt19937` 或 `BCryptGenRandom`）
2. **C4**: `AdjustResult` 添加 roommate_ids 重算逻辑
3. **M3**: 修复 `refreshTaskList` 未定义问题
4. **M7**: `ListTasks` / `ListDormitories` 添加认证
5. **H3**: 删除或实现 `MatchEngine::Match()`
6. **C2**: 改用 prepared statements 参数绑定
7. **M2**: 改用 UUID 或自增 ID
8. **Bug 15**: 完成 `AdminAdjust.vue` 的交换操作流程