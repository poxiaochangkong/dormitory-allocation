# Bug Report - 前后端对接问题汇总

> 生成时间：2026-05-25（更新于 2026-05-31）
> 范围：前端 `frontend-matching` 与 后端 `backend` 的 API 对接问题、UI 缺陷

---

## ✅ 已修复的 Bug

### ~~Bug 1: AdminDashboard 任务列表不显示~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 第 167-171 行

**原因**: `loadTasks()` 中数据格式不匹配。后端返回 `{ tasks: [...] }`，前端直接用 `Array.isArray(data)` 判断，永远为 false。

**修复**: 改为 `Array.isArray(data?.tasks) ? data.tasks : (Array.isArray(data) ? data : [])`

---

### ~~Bug 2: 创建任务时任务名称丢失~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 第 187 行

**原因**: 前端发送 `name` 字段，后端读取 `taskName` 字段。

**修复**: 前端改为发送 `taskName: newTask.name`

---

### ~~Bug 3: 任务表格"任务名称"列不显示~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 第 50 行

**原因**: `<el-table-column prop="name">` 与后端返回字段 `taskName` 不匹配。

**修复**: 改为 `prop="taskName"`

---

### ~~Bug 4: 学生登录后跳转到不存在的路由~~ ✅ 已修复

**文件**: `frontend-matching/src/views/Login.vue` 第 132 行 + `frontend-matching/src/router/index.js` 第 36 行

**原因**: 学生登录后跳转到 `/student/home`，但路由表中无此路由，也没有 `StudentHome.vue` 组件。

**修复**: 跳转目标改为 `/student/questionnaire`

---

### ~~Bug 5: 查看任务结果数据格式不匹配~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 第 222 行

**原因**: 后端返回 `{ allocations: [...] }`，前端直接当数组使用。

**修复**: 改为从 `data.allocations` 读取

---

## 🔴 P0 — 严重缺陷（功能完全不可用）

### ~~Bug 6: 学生端所有页面无退出登录按钮~~ ✅ 已修复

**文件**: `Questionnaire.vue` / `ImmersiveScene.vue` / `StudentResult.vue`

**原因**: 三个学生页面均没有退出登录按钮。学生登录后无法退出，只能手动清除浏览器 localStorage。

**修复**: 在每个学生页面的 header 区域添加了红色「退出登录」按钮，清除 localStorage（token/userId/role）后跳转到 `/login`。

---

### ~~Bug 7: "返回大厅" 按钮指向不存在的路由~~ ✅ 已修复

**文件**: `Questionnaire.vue` / `ImmersiveScene.vue` / `StudentResult.vue`

**原因**: 最初 `/student/home` 路由不存在。后来路由表已添加 `/student/home` 路由和 `StudentHome.vue` 组件，但 ImmersiveScene 和 StudentResult 页面缺少"返回大厅"按钮。

**修复**: 确认 `/student/home` 路由已正确定义。在 ImmersiveScene 中添加了"返回大厅"按钮。Questionnaire 和 StudentResult 已有正确导航。

---

### ~~Bug 8: 创建任务表单字段与后端不匹配~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 创建任务对话框

**原因**:
- 前端发送: `{ taskName, college, similarityWeight, complementarityWeight, vetoSafetyWeight }`
- 后端 `CreateTask` 读取: `{ taskName, college, major, gender }`
- 后端不读取三个 weight 字段，前端缺少 `major` 和 `gender` 输入框

**修复**: 采用方案 A — 移除了无效的权重滑块，添加了 `major`（专业）输入框和 `gender`（性别限制）下拉选择框。表单数据 `newTask` 改为 `{ taskName, college, major, gender }`，与后端完全对齐。

---

### ~~Bug 9: 任务结果表格列定义与后端数据不匹配~~ ✅ 已修复

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 结果对话框

**原因**: 
- 前端期望 `userIds` 数组列（按宿舍分组）
- 后端 `GetTaskResult` 返回的是**每行一个学生**的扁平结构：`{ studentNo, building, roomNumber, totalScore, explanationText }`

**修复**: 结果表格改为扁平列表，列定义为：学号（`studentNo`）、楼栋（`building`）、房间号（`roomNumber`）、匹配分（`totalScore`）、匹配说明（`explanationText`）。同时添加了 `resultTaskName` 展示任务名称。

---

## 🟡 P1 — 中等缺陷（功能缺失或体验差）

### ~~Bug 10: 管理员无删除任务功能~~ ✅ 已修复

**修复**:
1. 后端新增 `DELETE /api/admin/allocation/task/:taskId` 端点（`AdminService::DeleteTask`）
2. 前端添加 `deleteTask(taskId)` API 函数和删除按钮（带确认弹窗）

---

### ~~Bug 11: 管理员无用户管理页面~~ ✅ 已修复

**修复**: AdminDashboard 中添加了用户管理区域：
- 表格展示用户列表（学号、学院、角色）
- 每行有"删除"按钮（带确认弹窗，不可删除管理员）
- "设为管理员"按钮（管理员权限转让，转让后自动退出登录）
- 前端新增 `deleteUser()`、`transferAdmin()` API 函数

---

### ~~Bug 12: 无批量导入学生 UI~~ ✅ 已修复

**修复**: AdminDashboard 添加了"批量导入学生"卡片和对话框：
- 点击"导入学生"弹出对话框
- 文本框粘贴 JSON 数组格式的学生数据
- 前端解析 JSON 后调用 `importStudents()` API
- 显示导入结果（成功/失败 + 导入数量）

---

### Bug 13: 无分配规则配置 UI

**问题**:
- 后端已实现 `POST /api/admin/allocation/rule/save`
- 前端 `api/index.js` 已定义 `saveAllocationRule()` 函数
- 但没有规则配置的 UI 页面

**修复方案**: 在管理后台添加分配规则配置区域。

---

### ~~Bug 14: 无导出分配结果按钮~~ ✅ 已修复

**修复**: AdminDashboard 任务表格操作列已添加"导出CSV"按钮，调用 `exportTaskResult()` 下载 CSV 文件。

---

## 🟢 P2 — 轻微缺陷（可用但需优化）

### Bug 15: AdminAdjust 无法手动调整学生分配结果

**文件**: `frontend-matching/src/views/AdminAdjust.vue`

**问题**: 
- 后端已实现 `POST /api/admin/allocation/task/adjust`（交换两个学生的宿舍，参数 `taskId + userId1 + userId2`）
- 前端页面有拖拽 UI 但实际无法使用：
  - 没有"选择学生 A → 选择学生 B → 交换宿舍"的操作流程
  - 没有将单个学生移动到指定宿舍的功能
  - 调整后无法保存到后端
- 管理员完全无法手动干预某个具体学生的分配结果

**修复方案**: 
1. 加载任务分配结果，展示按宿舍分组的学生列表
2. 提供"交换"操作：选择两个学生，点击交换按钮，调用 `adjustDormitory({ taskId, userId1, userId2 })`
3. 提供"移动"操作：选择一个学生和目标宿舍，将其移入（需后端新增移动接口，或通过交换模拟）

---

### Bug 16: 后端多个端点无前端对接

| 后端端点 | 功能 | 前端状态 |
|---------|------|---------|
| `GET /api/student/questionnaire/template` | 获取问卷模板 | 无 API 函数，无调用 |
| `GET /api/student/questionnaire/status/:userId` | 查询问卷填写状态 | 无 API 函数，无调用 |
| `GET /api/student/info/:userId` | 获取学生信息 | 无 API 函数，无调用 |
| `GET /api/admin/allocation/rules` | 获取分配规则列表 | 无 API 函数，无调用 |
| `GET /api/admin/dashboard/stats` | 仪表盘统计数据 | 无 API 函数，无调用 |
| `POST /api/admin/transfer` | 管理员权限转让 | ✅ 已添加 API 函数和管理员转让 UI |

---

### Bug 17: StudentResult 空状态提示不够友好

**文件**: `frontend-matching/src/views/StudentResult.vue`

**问题**: 管理员未运行分配任务时，只显示"暂无分配结果"，缺少引导用户返回完成问卷的提示。

---

## 📋 修复优先级总览

| 优先级 | Bug | 影响范围 | 修复难度 | 状态 |
|-------|-----|---------|---------|------|
| P0 | Bug 1: 任务列表不显示 | 管理员无法使用系统 | 简单 | ✅ 已修复 |
| P0 | Bug 2: 任务名称丢失 | 数据展示错误 | 简单 | ✅ 已修复 |
| P0 | Bug 3: 表格列不匹配 | UI 显示问题 | 简单 | ✅ 已修复 |
| P0 | Bug 4: 学生登录跳转错误 | 学生无法进入系统 | 简单 | ✅ 已修复 |
| P0 | Bug 5: 结果数据格式错误 | 结果无法显示 | 简单 | ✅ 已修复 |
| P0 | Bug 6: 学生端无退出登录 | 学生无法退出 | 简单 | ✅ 已修复 |
| P0 | Bug 7: 返回大厅路由不存在 | 路由错误 | 简单 | ✅ 已修复 |
| P0 | Bug 8: 创建任务字段不匹配 | 后端忽略权重值 | 中等 | ✅ 已修复 |
| P0 | Bug 9: 结果表格列定义错误 | 分配学生列为空 | 中等 | ✅ 已修复 |
| P1 | Bug 10: 无删除任务功能 | 管理功能不完整 | 中等（需前后端） | ✅ 已修复 |
| P1 | Bug 11: 无用户管理页面 | 管理功能不完整 | 中等 | ✅ 已修复 |
| P1 | Bug 12: 无导入学生 UI | 管理功能不完整 | 中等 | ✅ 已修复 |
| P1 | Bug 13: 无规则配置 UI | 管理功能不完整 | 中等 | ❌ 未修复 |
| P1 | Bug 14: 无导出结果按钮 | 管理功能不完整 | 简单 | ✅ 已修复 |
| P2 | Bug 15: AdminAdjust 对接 | 调整功能受限 | 中等 | ❌ 未修复 |
| P2 | Bug 16: 后端端点无前端 | 功能缺失 | 中等 | ❌ 未修复 |
| P2 | Bug 17: 空状态提示 | 用户体验 | 简单 | ❌ 未修复 |

---

## 📝 修改记录

- 2026-05-25: 初始版本，记录 Bug 1-7
- 2026-05-26: Bug 1-5 标记为已修复；新增 Bug 6-17（全面 UI 缺陷检查）；移除"学生端无注册入口"（确认 Login.vue 已有注册表单）
- 2026-05-26 P0修复轮: Bug 6-9 全部修复（添加退出按钮、确认路由、重写创建任务表单、重写结果表格），27 条单元测试全部通过
- 2026-05-31 高优先级修复轮: Bug 10-12, 14 全部修复；后端匹配算法重写（CalculateSimilarity 归一化欧氏距离 + CalculateComplementarity MBTI互补 + HasVetoConflict 行为标签匹配）；分配规则持久化存储（system_config 表）；前端新增导入学生UI、用户管理区域、deleteUser/transferAdmin API
