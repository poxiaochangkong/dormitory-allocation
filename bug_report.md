# Bug Report

> 更新时间：2026-06-02 晚（前端体验修复轮）
> 范围：前后端对接问题、静态代码分析发现的 bug、项目功能可行性评估
> 说明：仅列出未修复的 bug，基于对全部源代码的静态分析
> 本次修复：B4, B5, B6, C8, M8, M9, M10, L5, L6, L8, L9 — 11 个 bug 已修复

---

## 📋 项目现状总览

### 后端已完成功能
| 模块 | 功能 | 状态 |
|------|------|------|
| 学生服务 | 登录、注册、获取信息、问卷提交(支持前后端两种格式)、场景数据提交、匹配结果查询 | ✅ |
| 管理服务 | 登录、导入学生、用户管理(删除/转让管理员)、任务CRUD、执行匹配、导出CSV、人工调整 | ✅ |
| 匹配算法 | 一票否决过滤、相似度评分(欧氏距离6维度)、互补性评分(MBTI+社交)、贪心分配 | ✅ |
| 基础设施 | 数据库初始化(9张表+索引)、密码哈希(SHA-256+salt)、Bearer Token认证、日志(spdlog)、配置加载 | ✅ |
| HTTP服务 | CORS、Bearer Token认证、全部REST端点(20+) | ✅ |

### 前端已完成功能
| 页面 | 功能 | 状态 |
|------|------|------|
| Login.vue | 学生/管理员登录Tab切换、学生注册表单 | ✅ |
| StudentHome.vue | 学生大厅(三阶段导航卡片) | ✅ |
| Questionnaire.vue | 基础画像 + 传统生活习惯 + 一票否决 + 性格互补(4个折叠面板) | ✅ |
| ImmersiveScene.vue | 沉浸式场景9题采集(热区点击+弹窗问答) | ✅ |
| StudentResult.vue | 匹配结果展示 + 5维雷达图(echarts) | ✅ |
| AdminDashboard.vue | 管理控制台(统计卡片、任务列表、用户管理、批量导入、导出CSV) | ✅ |
| AdminAdjust.vue | 按宿舍分组展示 + 选择两名学生交换宿舍 | ✅ |
| API层 | 19个API函数定义(api/index.js) | ✅ |
| 路由/认证 | vue-router + 导航守卫(role-based) + axios拦截器(token注入/响应解包) | ✅ |

### 数据库表结构
| 表名 | 用途 | 关键字段 |
|------|------|---------|
| `user` | 用户表 | user_id, student_no, password, salt, gender, college, major, grade, dorm_type, role, token |
| `questionnaire` | 问卷表 | questionnaire_id, user_id, sleep_schedule, hygiene_level, noise_tolerance, temperature_preference, social_preference, gaming_behavior, mbti_type, raw_answers |
| `preference` | 偏好权重表 | preference_id, user_id, similarity_weight, complementarity_weight, veto_safety_weight |
| `veto` | 一票否决表 | veto_id, user_id, veto_item |
| `open_text_profile` | 开放文本表 | profile_id, user_id, self_description, roommate_expectation |
| `dormitory` | 宿舍表 | dorm_id, building, room_number, capacity, gender, college, is_available |
| `allocation_task` | 分配任务表 | task_id, task_name, college, major, gender, status, rule_config |
| `match_result` | 匹配结果表 | result_id, task_id, user_id, dorm_id, roommate_ids, total_score, similarity_score, complementarity_score, veto_risk_score, explanation_text |
| `system_config` | 系统配置表 | config_key, config_value |

### 待完成工作
1. **分配规则配置UI** — 后端已有 `POST /api/admin/allocation/rule/save`，前端缺少配置页面
2. **沉浸式场景数据未参与匹配** — 收集了9个维度数据但算法完全未使用
3. **问卷数据回填** — 已提交问卷的学生再次进入 Questionnaire 页面时表单为空
4. **问卷模板API未对接** — 后端有 `GET /api/student/questionnaire/template` 但前端无API函数
5. **问卷状态检查未使用** — 前端已定义 `getQuestionnaireStatus()` 但从未调用
6. **学生信息API未使用** — 前端已定义 `getStudentInfo()` 但从未调用

### 已修复（2026-06-02）
- ✅ 学生登录跳转路径 (B4+M10)
- ✅ freeBeds 空闲床位计算 (B5+L9)
- ✅ 问卷表单验证 (B6)
- ✅ 注册后自动登录 (L5)
- ✅ StudentResult 空状态引导 (L6)
- ✅ ImmersiveScene 按钮文案 + loading (M9+L8)
- ✅ StudentHome 步骤指示器动态化 (M8)
- ✅ AdminAdjust CDATA 标记 (C8)

### 项目功能可行性判断

**结论：功能上可行，但存在重要缺陷需要修复。**

- ✅ 核心架构（前后端分离、REST API、MySQL持久化）设计合理
- ✅ 认证机制（SHA-256+salt密码哈希、Bearer Token）基本可用
- ✅ 匹配算法框架完整（一票否决→相似度→互补性→贪心分配），可替换为真实算法
- ⚠️ 当前匹配算法是占位算法，但框架支持替换为更复杂的算法
- ⚠️ 前端问卷收集的数据维度多于算法使用的维度，存在数据浪费
- ⚠️ 沉浸式场景数据完全未被利用，需要扩展算法
- ❌ 多个安全漏洞（SQL注入、弱Token生成）需要修复才能上线

---

## 🔴 严重缺陷

### C1. SQL 注入 —— 所有 SQL 均使用字符串拼接

**文件**: `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`, `server/main.cpp`

**描述**: 所有 SQL 查询通过字符串拼接用户输入构建。虽然有 `Escape()` 函数处理单引号，但不能防御全部注入向量（反引号、null bytes、多字节字符攻击）。

**影响**: 数据库可能被攻破。

**当前状态**: ⚠️ 部分缓解 — `Escape()` 覆盖了最常见的单引号向量。完整防护需改用 prepared statements + 参数绑定，改动量较大。

---

### C2. Token 使用 `std::rand()` 生成 —— 非密码学安全

**文件**: `StudentService.cpp` (第93-106行), `AdminService.cpp` (第76-85行)

**描述**: 认证 Token 由 `std::rand()` + 毫秒时间戳种子生成。`std::rand()` 不是密码学安全的 PRNG，Token 值可预测。且 `std::srand()` 在每次调用时重设全局种子，两个并发登录请求若间隔很短可能得到相同Token。

**影响**: Token 预测 → 会话劫持。

---

### C3. `AdjustResult` 只交换 dorm_id，不更新 roommate_ids

**文件**: `AdminService.cpp` — `AdjustResult()` (第474-480行)

**描述**: 管理员调整分配结果时只更新了 `dorm_id`，未重新计算受影响房间的 `roommate_ids`，导致数据不一致。

```cpp
// 当前代码：只交换 dorm_id
db.Execute("UPDATE match_result SET dorm_id = '" + dorm_id2 + "' WHERE result_id = '" + result_id1 + "';");
db.Execute("UPDATE match_result SET dorm_id = '" + dorm_id1 + "' WHERE result_id = '" + result_id2 + "';");
// 缺失：更新两个房间所有学生的 roommate_ids
```

**影响**: 调整后室友列表数据错误，学生查看结果时看到错误的室友信息。

---

### C4. `ImportStudents` 总是生成新 user_id 且计数不准

**文件**: `AdminService.cpp` — `ImportStudents()` (第104-163行)

**描述**: 每次导入生成新 `user_id`（`GenId("u_")`）。若 `student_no` 已存在，`ON DUPLICATE KEY UPDATE` 会更新旧行（不会插入新行），但 `count++` 仍然递增，导入数量统计误导。

**影响**: 给管理员展示错误的导入计数（显示导入了N个，实际可能只有M个新增）。

---

### C5. `AuthenticateRequest` 中 token 未转义直接拼入 SQL

**文件**: `server/main.cpp` — `AuthenticateRequest()` (第76行)

**描述**: Token 从 HTTP Header 提取后直接拼接到 SQL 查询中，未经过 `Escape()` 处理：
```cpp
sql << "SELECT user_id, role FROM `user` WHERE token = '" << token << "';";
```

**影响**: 攻击者可通过构造恶意 Authorization Header 实现 SQL 注入。

---

### C6. `MatchEngine::LoadStudentProfiles` 中查询条件未转义

**文件**: `MatchEngine.cpp` — `LoadStudentProfiles()` (第52-56行)

**描述**: `college`、`major`、`gender` 参数直接拼接到 SQL 查询中，未经过任何转义：
```cpp
if (!college.empty())
    sql += " AND u.college = '" + college + "'";
```

**影响**: 虽然这些参数来自管理员创建任务时的输入（非直接用户输入），但仍存在注入风险。

---

### C7. `MatchEngine::ExecuteAllocation` 中 explanation_text 未转义

**文件**: `MatchEngine.cpp` — `ExecuteAllocation()` (第665-682行)

**描述**: `r.explanation_text` 包含 dorm building 和 room_number（来自数据库），直接拼入 SQL INSERT 语句。若 building/room_number 中含有单引号（如 "O'Brien Hall"），将导致 SQL 语法错误。

**影响**: 特殊楼栋名会导致匹配结果保存失败。

---

### C8. `AdminAdjust.vue` 文件包含 `<![CDATA[` 和 `]]>` 标记 ✅ 已修复 (2026-06-02)

---

### C9. `SubmitQuestionnaire` 每次 INSERT 新行，导致同一用户被重复匹配

**文件**: `StudentService.cpp` — `SubmitQuestionnaire()` (第315-332行)

**描述**: 每次提交问卷都执行 `INSERT INTO questionnaire`（无 UPSERT / ON DUPLICATE KEY UPDATE 逻辑）。学生重新提交问卷后，`questionnaire` 表中同一 `user_id` 会有**多行记录**。`MatchEngine::LoadStudentProfiles()` 用 `INNER JOIN questionnaire` 查询，会将同一学生返回**多次**，导致该学生被当作多个不同个体参与匹配分配。

```cpp
// 当前代码：始终 INSERT，无去重
sql << "INSERT INTO questionnaire "
    << "(questionnaire_id, user_id, sleep_schedule, ...) VALUES (...);";
```

**影响**: 🔴 极其严重 — 学生重新提交问卷后，匹配算法会将同一人分配到多个宿舍，严重破坏分配结果的正确性。

**修复建议**: 改为 `INSERT ... ON DUPLICATE KEY UPDATE` 或先 `DELETE` 再 `INSERT`（基于 `user_id` 唯一约束）。

---

## 🟠 高优先级

### B1. 沉浸式场景数据完全未被匹配算法使用

**文件**: `ImmersiveScene.vue` + `StudentService.cpp` + `MatchEngine.cpp`

**描述**: 前端 `ImmersiveScene.vue` 收集了9个维度的场景数据：
- `s30_acTemp` (空调温度, slider 16-30)
- `s31_bedAction` (作息动作, A/B/C)
- `s32_deskState` (桌面状态, A/B/C)
- `s33_doorKnock` (敲门态度, A/B/C)
- `s34_curtain` (窗帘采光, A/B/C)
- `s35_trash` (垃圾处理, A/B/C)
- `s36_wash` (洗漱时长, A/B/C)
- `s37_noiseDb` (噪声耐受, slider 30-100)
- `s38_balconyWet` (阳台潮湿, A/B/C)

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

**影响**: 前端问卷收集的数据不足以填充匹配算法所需的全部维度，导致3个维度使用默认值，降低匹配准确性。

---

### B3. `RunTask` 同步执行期间 db_mutex 阻塞所有请求

**文件**: `server/main.cpp` — `POST /api/admin/allocation/task/run/:taskId` (第463行)

**描述**: 匹配算法 `MatchEngine::ExecuteAllocation()` 是同步执行的，执行期间持有 `db_mutex`，阻塞所有其他数据库操作（包括登录、问卷提交等）。对于大量学生的匹配任务，可能阻塞数十秒甚至更长。

**影响**: 分配任务执行期间，整个系统不可用。

---

### B4. 学生登录后跳转到问卷页而非大厅 ✅ 已修复 (2026-06-02)

---

### B5. AdminDashboard `freeBeds` 计算的是总床位数而非空闲床位数 ✅ 已修复 (2026-06-02)
— 改为 `totalCapacity - 最新已完成任务的已分配学生数`，同时修复 L9 变量遮蔽

---

### B6. Questionnaire.vue 只验证了 q02_sleepTime 一个字段 ✅ 已修复 (2026-06-02)
— 扩展为 7 个必填项验证：性别、学院、专业、入睡时间、卫生水平、MBTI、社交偏好

---

### B7. 已提交问卷的学生再次进入 Questionnaire 页面表单为空

**文件**: `Questionnaire.vue`

**描述**: 页面没有 `onMounted` 加载已提交的问卷数据回填表单。已提交问卷的学生再次访问时看到空白表单，可能误以为未提交过。

**影响**: 用户体验混乱，可能导致重复提交。

---

### B8. RunTask 异常时 status 永久停留在 'running'

**文件**: `AdminService.cpp` — `RunTask()` (第370-374行)

**描述**: 任务状态先更新为 `running`，然后执行匹配算法。若算法抛出异常，状态不会回滚为 `pending` 或标记为 `failed`，永久停留在 `running`。

```cpp
db.Execute("UPDATE allocation_task SET status = 'running' ...");
auto results = MatchEngine::ExecuteAllocation(...); // 若此处异常，status 永远是 running
```

**影响**: 异常任务无法重新执行（被 "Task is already running" 阻止），也无法被正确管理。

---

### B9. axios timeout 30秒可能不够 RunTask 使用

**文件**: `frontend-matching/src/utils/request.js` (第8行)

**描述**: axios 请求超时设置为 30 秒。`RunTask` 端点同步执行匹配算法，学生数量较多时可能超过 30 秒，导致前端超时错误而后端仍在执行。

**影响**: 前端显示超时错误，但后端任务实际已完成。

---

### B10. `MatchEngine::GreedyAssign` 只处理4人宿舍的2人初始配对

**文件**: `MatchEngine.cpp` — `GreedyAssign()` (第401-563行)

**描述**: 算法先找最佳2人配对，再填充剩余位置。但当所有剩余学生与现有组的平均分都 < 0 时（如都有否决冲突），会停止填充，导致宿舍可能只有2人而未住满4人。

```cpp
if (best_k < 0 || best_group_score < 0)
    break; // No suitable student found — 宿舍未住满就停止
```

**影响**: 宿舍利用率低，部分宿舍只有2人，其他学生可能被分配到"溢出"位置。

---

### B11. `SubmitSceneData` 在无问卷时静默丢弃数据

**文件**: `StudentService.cpp` — `SubmitSceneData()` (第478-503行)

**描述**: 若学生未提交基础问卷就直接提交场景数据，`SELECT ... FROM questionnaire WHERE user_id = ...` 返回空集，函数跳过数据库写入逻辑，但仍返回 `{status: "ok"}`。前端误以为提交成功。

```cpp
auto rs = db.ExecuteQuery("SELECT questionnaire_id, raw_answers FROM questionnaire ...");
if (rs->next()) {
    // 只有查到问卷才会写入场景数据
    ...
}
// 无问卷时直接返回 ok，数据丢失
nlohmann::json result;
result["status"] = "ok";
return result.dump();
```

**影响**: 用户完成沉浸式问卷后，若未先提交基础问卷，数据将被静默丢弃，且前端不会提示错误。

**修复建议**: 检查问卷是否存在，若不存在应抛出异常或返回错误提示。

---

### B12. `DeleteTask` 不检查任务是否正在运行

**文件**: `AdminService.cpp` — `DeleteTask()` (第605-628行)

**描述**: 删除任务时未检查任务状态。若任务状态为 `running`（正在执行匹配），删除操作会先删除 `match_result` 表中的相关记录，导致正在执行的 `ExecuteAllocation` 写入失败或数据不一致。

**影响**: 删除运行中的任务可能导致数据库中残留孤儿记录或写入错误。

**修复建议**: 删除前检查 `status != 'running'`，若为运行中则拒绝删除。

---

## 🟡 中等优先级

### M1. `GenerateToken()` 调用 `std::srand()` 影响全局随机状态

**文件**: `StudentService.cpp` (第98行), `AdminService.cpp` (第79行)

**描述**: 每次调用 `std::srand()` 重设全局随机数种子。若两个登录请求间隔很短，可能得到相同种子 → 相同 Token。

**影响**: Token 碰撞 → 会话劫持。

---

### M2. `GenId()` 用毫秒 + rand 生成 ID

**文件**: `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`

**描述**: 时间戳毫秒 + `rand() % 10000`。高并发下 ID 会碰撞。

**影响**: 主键重复错误或静默数据覆盖。

---

### M3. 前端问卷多个字段未在匹配算法中使用

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

### M4. `GetMatchResult` 返回虚假子分数

**文件**: `StudentService.cpp` — `GetMatchResult()` (第567-568行)

**描述**: 返回的 `hygieneConsistencyScore` 和 `scheduleOverlapScore` 由公式 `sim_score * 0.9 + 0.05`、`sim_score * 0.85 + 0.1` 计算，并非算法的真实度量。

```cpp
result["hygieneConsistencyScore"] = sim_score * 0.9 + 0.05;
result["scheduleOverlapScore"] = sim_score * 0.85 + 0.1;
```

**影响**: 前端雷达图展示误导性数据。

---

### M5. `ListTasks` 和 `ListDormitories` 无认证

**文件**: `server/main.cpp` (第592-624行)

**描述**: `GET /api/admin/tasks` 和 `GET /api/admin/dormitories` 没有认证检查。代码注释说明这是"to match existing frontend behavior"。

**影响**: 任何人可以未登录查看所有任务和寝室信息。

---

### M6. `ExportResult` CSV 未转义字段中的逗号和换行

**文件**: `AdminService.cpp` — `ExportResult()` (第503-517行)

**描述**: 若 college、major 等字段含逗号或换行符，CSV 格式会被破坏。标准 CSV 格式要求含特殊字符的字段用双引号包裹。

**影响**: 导出的 CSV 在 Excel 等工具中解析错误。

---

### M7. `SaveAllocationRule` 保存 JSON 时转义不完整

**文件**: `AdminService.cpp` — `SaveAllocationRule()` (第284-305行)

**描述**: `rule_json` 经 `Escape()` 只转义了单引号，JSON 中的反斜杠等 SQL 特殊字符未被处理。

**影响**: 保存复杂规则时可能 SQL 报错或注入。

---

### M8. StudentHome.vue 步骤指示器硬编码为阶段1 ✅ 已修复
— 已改为 `:active="activeStep"` 动态绑定

---

### M9. ImmersiveScene.vue 按钮文案误导 ✅ 已修复
— 已改为 "完成 → 进入权重调节"

---

### M10. 前端 router 守卫中已登录学生重定向路径错误 ✅ 已修复 (2026-06-02)

---

### M11. `AdjustResult` 交换后 `explanation_text` 仍引用旧宿舍信息

**文件**: `AdminService.cpp` — `AdjustResult()` (第474-480行)

**描述**: 管理员交换两名学生的宿舍后，只更新了 `dorm_id` 字段，未更新 `explanation_text`。`explanation_text` 包含 "Assigned to Building X Room Y" 的文本描述，交换后该文本与新的 `dorm_id` 不一致。

**影响**: 学生查看匹配结果时，解释文字中的宿舍楼栋/房间号与实际分配不符，造成困惑。与 C3 同属 AdjustResult 的缺陷，建议一并修复。

---

### M12. `AdminLogin` 响应字段与 `StudentLogin` 不一致

**文件**: `AdminService.cpp` — `Login()` (第94-101行) vs `StudentService.cpp` — `Login()` (第147-158行)

**描述**: Admin 登录响应仅返回 `{ userId, studentNo, role, gender, token }`，而 Student 登录响应还包含 `{ college, major, grade, dormType }`。若前端代码复用同一渲染逻辑（如显示用户信息），admin 端会因缺少字段而报错或显示空值。

```cpp
// AdminService::Login 响应（缺少 college/major/grade/dormType）
result["userId"] = user_id;
result["studentNo"] = student_no;
result["role"] = ...;
result["gender"] = ...;
result["token"] = token;
```

**影响**: 前端若统一解析登录响应中的 college/major 等字段，admin 登录会拿到 undefined。

---

## 🔵 低优先级

### L1. 注册无密码强度校验

**文件**: `StudentService.cpp` — `Register()`

**描述**: 任何密码均接受，仅通过 `password.empty()` 拦空白串。

---

### L2. 登录端点无速率限制

**文件**: `server/main.cpp`

**描述**: `/api/student/login` 和 `/api/admin/login` 无暴力破解防护。

---

### L3. 单锁 `db_mutex` 串行化所有请求

**文件**: `server/main.cpp`

**描述**: 单个 mutex 守护所有数据库访问，高负载下成为瓶颈。

---

### L4. Token 永不过期

**文件**: `StudentService.cpp`, `AdminService.cpp`

**描述**: Token 无过期机制，除非重新登录（生成新 Token 隐式作废旧 Token）。

---

### L5. 注册成功后不自动登录 ✅ 已修复 (2026-06-02)
— 注册成功后自动调用登录并跳转

---

### L6. StudentResult 空状态缺少引导 ✅ 已修复 (2026-06-02)
— 增加引导文字"请先在大厅完成全部 5 步问卷，然后由管理员执行分配任务"

---

### L7. `MatchEngine::DeriveBehaviorTags` 无法推导部分否决项

**文件**: `MatchEngine.cpp` — `DeriveBehaviorTags()` (第310-354行)

**描述**: 以下否决项无法从问卷数据推导：
- `smoke_alcohol` — 问卷未收集吸烟/饮酒信息
- `opposite_sex` — 无法从问卷推导
- `pets` — 问卷未收集养宠信息

**影响**: 这些否决项永远不会触发冲突检测，一票否决机制对这些项目无效。

---

### L8. `ImmersiveScene.vue` 按钮未绑定 loading 状态 ✅ 已修复
— `:loading="submitting"` 已绑定

---

### L9. `AdminDashboard.vue` 局部变量遮蔽外部 ref ✅ 已修复 (2026-06-02)
— 局部变量 `userList` 重命名为 `users`，随 B5 一并修复

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
| 7 | `POST /api/admin/students/import` | JSON array | JSON array + ON DUPLICATE KEY | ⚠️ 计数不准 |
| 8 | `POST /api/admin/allocation/task/create` | `{ taskName, college, major, gender }` | 同左 | ✅ 匹配 |
| 9 | `POST /api/admin/allocation/task/run/:taskId` | path param | 同步执行匹配算法 | ⚠️ 同步阻塞 |
| 10 | `GET /api/admin/allocation/task/result/:taskId` | path param | 返回 allocations 数组 | ✅ 匹配 |
| 11 | `POST /api/admin/allocation/task/adjust` | `{ taskId, userId1, userId2 }` | 交换 dorm_id | ⚠️ 不更新 roommate_ids |
| 12 | `GET /api/admin/allocation/task/export/:taskId` | path param | 返回 CSV text | ✅ 匹配 |
| 13 | `DELETE /api/admin/allocation/task/:taskId` | path param | 删除任务及结果 | ✅ 匹配 |
| 14 | `GET /api/admin/tasks` | 无 | 无认证 | ⚠️ 无认证 |
| 15 | `GET /api/admin/dormitories` | 无 | 无认证 | ⚠️ 无认证 |
| 16 | `POST /api/admin/dormitories` | `{ building, roomNumber, capacity, gender }` | 同左 | ✅ 匹配 |
| 17 | `GET /api/admin/users` | Bearer token (admin) | admin 认证 | ✅ 匹配 |
| 18 | `DELETE /api/admin/users/:userId` | path param | admin 认证 | ✅ 匹配 |
| 19 | `POST /api/admin/transfer` | `{ targetUserId }` | admin 认证 | ✅ 匹配 |
| 20 | `POST /api/admin/allocation/rule/save` | JSON rule | 存入 system_config | ✅ 匹配(但无UI) |
| 21 | `GET /api/student/questionnaire/template` | — | 返回问卷模板 | ❌ 前端未定义API函数 |
| 22 | `GET /api/student/questionnaire/status/:userId` | — | 查询问卷状态 | ❌ 前端有API但未调用 |
| 23 | `GET /api/student/info/:userId` | — | 获取学生信息 | ❌ 前端有API但未调用 |

---

## 修复建议（按优先级）

### 🔴 必须修复（影响安全性或数据正确性）— 7 项

1. **C9**: `SubmitQuestionnaire` 改为 `INSERT ... ON DUPLICATE KEY UPDATE`，防止问卷重复行导致同一学生被多次匹配
2. **C5**: `AuthenticateRequest` 中 token 应使用 `Escape()` 转义
3. **C6**: `MatchEngine::LoadStudentProfiles` 中查询参数应转义
4. **C7**: `MatchEngine::ExecuteAllocation` 中 explanation_text 应使用 `Escape()`
5. **C2 + M1**: 使用 CSPRNG 生成 Token（`std::random_device` + `std::mt19937`）
6. **C3 + M11**: `AdjustResult` 添加 roommate_ids 重算逻辑，同时更新 explanation_text
7. **C4**: `ImportStudents` 区分新增和更新的计数

### 🟠 功能完善（影响核心功能）— 6 项

8. **B1**: 将沉浸式场景数据整合到匹配算法中
9. **B2**: 补充前端问卷字段映射（noise_tolerance, temperature_preference, gaming_behavior）
10. **B8**: `RunTask` 添加 try-catch，异常时将 status 设为 'failed'
11. **B3**: 将匹配算法改为异步执行或使用独立线程
12. **B10**: 改进贪心算法，确保宿舍尽量住满
13. **B11**: `SubmitSceneData` 在无问卷时应返回错误而非静默丢弃
14. **B12**: `DeleteTask` 添加 `status != 'running'` 检查

### 🟡 用户体验优化 — 4 项

15. **B7**: Questionnaire 加载已提交数据回填
16. **M5**: `ListTasks` / `ListDormitories` 添加认证
17. **M12**: AdminLogin 响应补充 `college/major/grade/dormType` 字段
18. **B9**: RunTask 前端超时 30 秒可能不够，建议增大或改用轮询

### 🔵 代码质量 — 8 项

19. **M2**: 改用 UUID 或自增 ID
20. **M4**: 移除虚假子分数或实现真实的子维度评分
21. **M6**: CSV 导出使用标准转义（双引号包裹含逗号的字段）
22. **M7**: `SaveAllocationRule` 使用 prepared statement 或更完善的转义
23. **L1**: 注册添加密码强度校验
24. **L2**: 添加登录速率限制
25. **L3**: 考虑连接池替代单锁
26. **L4**: 添加 Token 过期机制
27. **L7**: 完善问卷以收集否决项推导所需数据（吸烟、宠物等）

### ✅ 已修复（2026-06-02 前端体验修复轮）— 11 项

- C8: CDATA 已移除
- B4 + M10: 登录跳转 `/student/home`
- B5 + L9: freeBeds 正确计算 + 变量遮蔽
- B6: 7 项必填验证
- M8: 步骤指示器动态化
- M9: 按钮文案修正
- L5: 注册自动登录
- L6: 空状态引导
- L8: loading 绑定
