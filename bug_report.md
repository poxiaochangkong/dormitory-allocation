# Bug Report

> 生成时间：2026-06-02（更新）
> 范围：前后端对接问题、静态代码分析发现的 bug
> 说明：仅列出未修复的 bug，基于对全部源代码的静态分析

---

## 📋 项目现状总览

### 后端已完成功能
| 模块 | 功能 | 状态 |
|------|------|------|
| 学生服务 | 登录、注册、获取信息、问卷提交、场景数据提交、匹配结果查询 | ✅ |
| 管理服务 | 登录、导入学生、用户管理、任务CRUD、执行匹配、导出CSV、人工调整 | ✅ |
| 匹配算法 | 一票否决过滤、相似度评分(欧氏距离)、互补性评分(MBTI+社交)、贪心分配 | ✅ |
| 基础设施 | 数据库初始化、密码哈希(CryptoUtil)、日志(spdlog)、配置加载 | ✅ |
| HTTP服务 | CORS、Bearer Token认证、全部REST端点 | ✅ |

### 前端已完成功能
| 页面 | 功能 | 状态 |
|------|------|------|
| Login.vue | 学生/管理员登录、学生注册 | ✅ |
| StudentHome.vue | 学生大厅(三阶段导航) | ✅ |
| Questionnaire.vue | 基础画像 + 一票否决问卷 | ✅ |
| ImmersiveScene.vue | 沉浸式场景9题采集 | ✅ |
| StudentResult.vue | 匹配结果展示 + 雷达图 | ✅ |
| AdminDashboard.vue | 管理控制台(任务管理、用户管理、导入、导出) | ✅ |
| AdminAdjust.vue | 人工交换宿舍 | ✅ |
| API层 | 所有API函数定义 | ✅ |

### 待完成工作
1. **分配规则配置UI** — 后端已有 `POST /api/admin/allocation/rule/save`，前端缺少配置页面
2. **沉浸式场景数据未参与匹配** — 收集了9个维度数据但算法完全未使用
3. **问卷数据回填** — 已提交问卷的学生再次进入页面时表单为空
4. **问卷模板API** — 后端有 `GET /api/student/questionnaire/template` 但前端无API函数

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

### C6. `AuthenticateRequest` 中 token 未转义直接拼入 SQL

**文件**: `server/main.cpp` — `AuthenticateRequest()` (第76行)

**描述**: Token 从 HTTP Header 提取后直接拼接到 SQL 查询中，未经过 `Escape()` 处理：
```cpp
sql << "SELECT user_id, role FROM `user` WHERE token = '" << token << "';";
```

**影响**: 攻击者可通过构造恶意 Authorization Header 实现 SQL 注入。

---

### C7. `MatchEngine::LoadStudentProfiles` 中查询条件未转义

**文件**: `MatchEngine.cpp` — `LoadStudentProfiles()` (第52-56行)

**描述**: `college`、`major`、`gender` 参数直接拼接到 SQL 查询中，未经过任何转义：
```cpp
if (!college.empty())
    sql += " AND u.college = '" + college + "'";
```

**影响**: 虽然这些参数来自管理员创建任务时的输入（非直接用户输入），但仍存在注入风险。

---

### C8. `MatchEngine::ExecuteAllocation` 中 explanation_text 未转义

**文件**: `MatchEngine.cpp` — `ExecuteAllocation()` (第680行)

**描述**: `r.explanation_text` 包含 dorm building 和 room_number（来自数据库），直接拼入 SQL INSERT 语句。若 building/room_number 中含有单引号（如 "O'Brien Hall"），将导致 SQL 语法错误。

**影响**: 特殊楼栋名会导致匹配结果保存失败。

---

## 🟠 高优先级

### B1. 🔴 沉浸式场景数据完全未被匹配算法使用

**文件**: `ImmersiveScene.vue` + `StudentService.cpp` + `MatchEngine.cpp`

**描述**: 前端 `ImmersiveScene.vue` 收集了9个维度的场景数据：
- `s30_acTemp` (空调温度)
- `s31_bedAction` (作息动作)
- `s32_deskState` (桌面状态)
- `s33_doorKnock` (敲门态度)
- `s34_curtain` (窗帘采光)
- `s35_trash` (垃圾处理)
- `s36_wash` (洗漱时长)
- `s37_noiseDb` (噪声耐受)
- `s38_balconyWet` (阳台潮湿)

但后端 `SubmitSceneData` 只将数据存入 `questionnaire.raw_answers.sceneData`，`MatchEngine` 完全没有读取或使用这些数据。匹配算法仅使用传统问卷的6个维度（sleep_schedule, hygiene_level, noise_tolerance, temperature_preference, social_preference, gaming_behavior）。

**影响**: 用户花时间完成的沉浸式问卷对匹配结果毫无影响，浪费用户体验。

---

### B2. 前端格式问卷缺少多个字段映射

**文件**: `StudentService.cpp` — `SubmitQuestionnaire()` (第274-296行)

**描述**: 当检测到前端格式（含 `traditionalHabits`、`personality` 等字段）时，以下字段无法正确映射：

| 前端字段 | 后端字段 | 状态 |
|---------|---------|------|
| `q02_sleepTime` (A/B/C/D) | `sleep_schedule` | ✅ 有映射 |
| `q06_hygiene` (A/B/C) | `hygiene_level` | ✅ 有映射 |
| `p22_socialEnergy` (A/B/C) | `social_preference` | ✅ 有映射 |
| `p21_mbti` | `mbti_type` | ✅ 直接传递 |
| — | `noise_tolerance` | ❌ 使用默认值3 |
| — | `temperature_preference` | ❌ 使用默认值24 |
| — | `gaming_behavior` | ❌ 使用空字符串 |

**影响**: 前端问卷收集的数据不足以填充匹配算法所需的全部维度，导致部分维度使用默认值，降低匹配准确性。

---

### H2. `GreedyAssign` 中 `dorm_idx` 变量多余

**文件**: `MatchEngine.cpp` — `GreedyAssign()` (第425-429行)

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

### B3. `RunTask` 同步执行期间 db_mutex 阻塞所有请求

**文件**: `server/main.cpp` — `POST /api/admin/allocation/task/run/:taskId` (第463行)

**描述**: 匹配算法 `MatchEngine::ExecuteAllocation()` 是同步执行的，执行期间持有 `db_mutex`，阻塞所有其他数据库操作（包括登录、问卷提交等）。对于大量学生的匹配任务，可能阻塞数十秒甚至更长。

**影响**: 分配任务执行期间，整个系统不可用。

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

### ~~M3. `AdminDashboard.vue` 调用了未定义的 `refreshTaskList()`~~ (已修复)

**文件**: `AdminDashboard.vue`

**描述**: 经代码审查，当前版本使用 `loadTasks()` 函数，不再引用 `refreshTaskList()`。此Bug已修复。

---

### M4. 前端问卷多个字段未在匹配算法中使用

**文件**: `Questionnaire.vue` / `StudentService.cpp`

**描述**: 前端问卷收集了以下字段，但后端仅存入 `raw_answers` JSON，匹配算法未使用：
- `q03_wakeTime` (起床时间)
- `q07_laundry` (换洗衣物处理)
- `q08_studyPlace` (宿舍学习频率)
- `p23_idealRelation` (理想室友关系)
- `p26_plan` (计划性)
- `p28_rules` (寝室公约态度)

**影响**: 用户偏好数据被收集但不参与匹配。

---

### M6. `GetMatchResult` 返回虚假子分数

**文件**: `StudentService.cpp` — `GetMatchResult()` (第567-568行)

**描述**: 返回的 `hygieneConsistencyScore` 和 `scheduleOverlapScore` 由公式 `sim_score * 0.9 + 0.05`、`sim_score * 0.85 + 0.1` 计算，并非算法的真实度量。

**影响**: 前端雷达图展示误导性数据。

---

### M7. `ListTasks` 和 `ListDormitories` 无认证

**文件**: `server/main.cpp` (第592-624行)

**描述**: `GET /api/admin/tasks` 和 `GET /api/admin/dormitories` 没有认证检查。代码注释说明这是"to match existing frontend behavior"。

**影响**: 任何人可以未登录查看所有任务和寝室。

---

### M8. `ExportResult` CSV 未转义字段中的逗号和换行

**文件**: `AdminService.cpp` — `ExportResult()` (第503-517行)

**描述**: 若 college、major 等字段含逗号或换行符，CSV 格式会被破坏。标准 CSV 格式要求含特殊字符的字段用双引号包裹。

**影响**: 导出的 CSV 在 Excel 等工具中解析错误。

---

### B4. 学生登录后跳转到问卷页而非大厅

**文件**: `Login.vue` (第132行)

**描述**: 学生登录成功后直接跳转到 `/student/questionnaire`，应跳转到 `/student/home`（大厅），让用户自主选择先做哪个问卷。

```javascript
// 当前代码
router.push('/student/questionnaire')
// 应改为
router.push('/student/home')
```

**影响**: 学生登录后直接进入问卷页，无法选择沉浸式场景问卷。

---

### B5. AdminDashboard `freeBeds` 计算的是总床位数而非空闲床位数

**文件**: `AdminDashboard.vue` (第313行)

**描述**: 当前计算方式为所有宿舍 capacity 之和：
```javascript
stats.freeBeds = dormList.reduce((sum, d) => sum + (d.capacity || 0), 0)
```
这计算的是总床位数，而非减去已分配学生后的空闲床位数。

**影响**: 管理员看到的"空闲宿舍床位"数据不准确。

---

### B6. Questionnaire.vue 只验证了 q02_sleepTime 一个字段

**文件**: `Questionnaire.vue` (第204行)

**描述**: 提交问卷时仅验证入睡时间（`q02_sleepTime`）是否填写，其他必填字段（性别、学院、专业等）均未验证。

**影响**: 用户可以提交不完整的问卷数据。

---

### B7. 已提交问卷的学生再次进入 Questionnaire 页面表单为空

**文件**: `Questionnaire.vue`

**描述**: 页面 `onMounted` 中没有加载已提交的问卷数据回填表单。已提交问卷的学生再次访问时看到空白表单，可能误以为未提交过。

**影响**: 用户体验混乱，可能导致重复提交。

---

### B8. RunTask 异常时 status 永久停留在 'running'

**文件**: `AdminService.cpp` — `RunTask()` (第371-374行)

**描述**: 任务状态先更新为 `running`，然后执行匹配算法。若算法抛出异常，状态不会回滚为 `pending` 或标记为 `failed`，永久停留在 `running`。

```cpp
db.Execute("UPDATE allocation_task SET status = 'running' ...");
auto results = MatchEngine::ExecuteAllocation(...); // 若此处异常，status 永远是 running
```

**影响**: 异常任务无法重新执行（被 "Task is already running" 阻止），也无法被正确管理。

---

### Bug 13. 无分配规则配置 UI

**文件**: `AdminDashboard.vue`

**问题**: 后端已实现 `POST /api/admin/allocation/rule/save`，前端 `api/index.js` 已定义 `saveAllocationRule()`，但缺少规则配置 UI 页面。

**影响**: 管理员无法通过界面配置分配规则。

---

### ~~Bug 15. AdminAdjust 无法手动调整学生分配~~ (已修复)

**文件**: `AdminAdjust.vue`

**描述**: 经代码审查，当前版本 AdminAdjust.vue 已实现完整的交换操作流程：选择两名学生 → 点击交换 → 调用 `adjustDormitory` API → 重新加载数据。此Bug已修复。

---

### Bug 16. 后端部分端点无前端对接（已部分修复）

| 后端端点 | 功能 | 前端API函数 | 前端调用 | 状态 |
|---------|------|-----------|---------|------|
| `GET /api/student/questionnaire/template` | 获取问卷模板 | ❌ 无 | ❌ 无 | 未对接 |
| `GET /api/student/questionnaire/status/:userId` | 查询问卷状态 | ✅ 已定义 | ❌ 无调用 | 未使用 |
| `GET /api/student/info/:userId` | 获取学生信息 | ✅ 已定义 | ❌ 无调用 | 未使用 |

**影响**: 问卷状态检查和学生信息回填功能无法使用。

---

### Bug 17. StudentResult 空状态提示不够友好

**文件**: `StudentResult.vue`

**问题**: 未分配时只显示"暂无分配结果，请等待管理员执行分配任务"，缺少引导用户完成问卷的提示。

**影响**: 用户体验欠佳。

---

## 🔵 低优先级

### L1. 注册无密码强度校验

**文件**: `StudentService.cpp` — `Register()`

**描述**: 任何密码均接受，仅通过 `password.empty()` 拦空白串。

---

### ~~L2. 前端 API baseURL 硬编码为 `localhost:8080`~~ (已修复)

**文件**: `frontend-matching/src/utils/request.js`

**描述**: 经代码审查，当前 axios baseURL 设置为 `/api`（相对路径），通过 Vite 代理转发到后端。此Bug已修复。

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

### B9. axios timeout 30秒可能不够 RunTask 使用

**文件**: `frontend-matching/src/utils/request.js` (第8行)

**描述**: axios 请求超时设置为 30 秒。`RunTask` 端点同步执行匹配算法，学生数量较多时可能超过 30 秒，导致前端超时错误而后端仍在执行。

**影响**: 前端显示超时错误，但后端任务实际已完成。

---

### B10. 注册成功后不自动登录

**文件**: `Login.vue` (第158行)

**描述**: 注册成功后仅显示成功消息并自动填入学号，用户仍需手动输入密码登录。

**影响**: 用户体验小问题。

---

## 前后端接口对接详细分析

| # | 端点 | 前端发送 | 后端接收/处理 | 状态 |
|---|------|---------|-------------|------|
| 1 | `POST /api/student/login` | `{ studentNo, password }` | `{ studentNo, password }` | ✅ 匹配 |
| 2 | `POST /api/student/register` | `{ studentNo, password, gender, college, major, grade }` | 同左 | ✅ 匹配 |
| 3 | `POST /api/student/questionnaire/submit` | `{ userId, basicInfo, traditionalHabits, vetoSettings, personality }` | 检测 `traditionalHabits` 字段走前端格式映射 | ⚠️ 部分字段缺失映射 |
| 4 | `POST /api/student/scene/submit` | `{ userId, s30_acTemp, s31_bedAction, ... }` | 存入 raw_answers，不参与匹配 | ⚠️ 数据浪费 |
| 5 | `GET /api/student/match-result/:userId` | Bearer token + path param | Token 认证 + 查询 | ✅ 匹配 |
| 6 | `POST /api/admin/login` | `{ studentNo, password }` | `{ studentNo, password }` + role='admin' | ✅ 匹配 |
| 7 | `POST /api/admin/allocation/task/create` | `{ taskName, college, major, gender }` | 同左 | ✅ 匹配 |
| 8 | `POST /api/admin/allocation/task/run/:taskId` | path param | 同步执行匹配算法 | ⚠️ 同步阻塞 |
| 9 | `GET /api/admin/allocation/task/result/:taskId` | path param | 返回 allocations 数组 | ✅ 匹配 |
| 10 | `POST /api/admin/allocation/task/adjust` | `{ taskId, userId1, userId2 }` | 交换 dorm_id | ⚠️ 不更新 roommate_ids |
| 11 | `GET /api/admin/allocation/task/export/:taskId` | path param | 返回 CSV text | ✅ 匹配 |
| 12 | `DELETE /api/admin/allocation/task/:taskId` | path param | 删除任务及结果 | ✅ 匹配 |
| 13 | `POST /api/admin/students/import` | JSON array | JSON array + ON DUPLICATE KEY | ⚠️ 计数不准 |
| 14 | `GET /api/admin/tasks` | 无 | 无认证 | ⚠️ 无认证 |
| 15 | `GET /api/admin/dormitories` | 无 | 无认证 | ⚠️ 无认证 |
| 16 | `POST /api/admin/dormitories` | `{ building, roomNumber, capacity, gender }` | 同左 | ✅ 匹配 |
| 17 | `GET /api/admin/users` | Bearer token (admin) | admin 认证 | ✅ 匹配 |
| 18 | `DELETE /api/admin/users/:userId` | path param | admin 认证 | ✅ 匹配 |
| 19 | `POST /api/admin/transfer` | `{ targetUserId }` | admin 认证 | ✅ 匹配 |
| 20 | `POST /api/admin/allocation/rule/save` | JSON rule | 存入 system_config | ✅ 匹配(但无UI) |

---

## 修复建议（按优先级）

### 🔴 必须修复（影响安全性或数据正确性）

1. **C6**: `AuthenticateRequest` 中 token 应使用 `Escape()` 转义
2. **C7**: `MatchEngine::LoadStudentProfiles` 中查询参数应转义
3. **C8**: `MatchEngine::ExecuteAllocation` 中 explanation_text 应使用 `Escape()`
4. **C3 + M1**: 使用 CSPRNG 生成 Token（`std::random_device` + `std::mt19937`）
5. **C4**: `AdjustResult` 添加 roommate_ids 重算逻辑
6. **C5**: `ImportStudents` 区分新增和更新的计数
7. **C2**: 长期改用 prepared statements 参数绑定

### 🟠 功能完善（影响核心功能）

8. **B1**: 将沉浸式场景数据整合到匹配算法中
9. **B2**: 补充前端问卷字段映射（noise_tolerance, temperature_preference, gaming_behavior）
10. **B8**: `RunTask` 添加 try-catch，异常时将 status 设为 'failed'
11. **B3**: 将匹配算法改为异步执行或使用独立线程
12. **H3**: 删除未实现的 `Match()` 声明

### 🟡 用户体验优化

13. **B4**: 学生登录后跳转到 `/student/home`
14. **B5**: `freeBeds` 计算减去已分配学生数
15. **B6**: Questionnaire 添加完整表单验证
16. **B7**: Questionnaire 加载已提交数据回填
17. **M7**: `ListTasks` / `ListDormitories` 添加认证
18. **Bug 13**: 实现分配规则配置 UI
19. **Bug 16**: 对接问卷模板API、问卷状态检查API

### 🔵 代码质量

20. **M2**: 改用 UUID 或自增 ID
21. **M6**: 移除虚假子分数或实现真实的子维度评分
22. **M8**: CSV 导出使用标准转义（双引号包裹含逗号的字段）
23. **L3**: 添加登录速率限制
24. **L4**: 考虑连接池替代单锁
25. **L5**: 添加 Token 过期机制