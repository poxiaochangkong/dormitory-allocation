# Bug Report - 前后端对接问题汇总

> 生成时间：2026-05-25（更新于 2026-05-26）
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

### Bug 6: 学生端所有页面无退出登录按钮

**文件**: `Questionnaire.vue` / `ImmersiveScene.vue` / `StudentResult.vue`

**问题**: 三个学生页面均没有退出登录按钮。学生登录后无法退出，只能手动清除浏览器 localStorage。

**影响**: 学生换号登录、多人共用设备时严重不便。

**修复方案**: 在每个学生页面的 header 区域添加「退出登录」按钮，清除 localStorage 后跳转到 `/login`。

---

### Bug 7: "返回大厅" 按钮指向不存在的路由

**文件**:
- `Questionnaire.vue` 第 11 行：`$router.push('/student/home')`
- `StudentResult.vue` 第 15、27 行：`$router.push('/student/home')`

**问题**: `/student/home` 路由不存在（路由表中未定义），也没有 `StudentHome.vue` 组件。

**影响**: 点击"返回大厅"按钮会导致 404 或路由错误。

**修复方案**: 将"返回大厅"改为跳转到实际存在的页面，如 `/student/questionnaire` 或 `/student/result`。

---

### Bug 8: 创建任务表单字段与后端不匹配

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 创建任务对话框

**问题**:
- 前端发送: `{ taskName, college, similarityWeight, complementarityWeight, vetoSafetyWeight }`
- 后端 `CreateTask` 读取: `{ taskName, college, major, gender }`
- 后端不读取三个 weight 字段，前端缺少 `major` 和 `gender` 输入框

**影响**:
1. 前端的权重滑块（相似度权重、互补权重、安全权重）完全无效，后端忽略这些值
2. 后端需要的 `major`（专业）和 `gender`（性别）字段无法输入

**修复方案**: 
- 方案 A：前端添加 `major` 和 `gender` 输入框，移除无效的 weight 滑块
- 方案 B：后端 `CreateTask` 增加对 weight 字段的读取和存储

---

### Bug 9: 任务结果表格列定义与后端数据不匹配

**文件**: `frontend-matching/src/views/AdminDashboard.vue` 结果对话框

**问题**: 
- 前端期望 `userIds` 数组列：`<el-table-column prop="userIds" label="分配学生" />`
- 后端 `GetTaskResult` 返回的是**每行一个学生**的扁平结构：`{ userId, dormId, building, roomNumber, studentNo, totalScore }`
- 没有 `userIds` 字段

**影响**: 结果表格中"分配学生"列始终为空。

**修复方案**: 改为按行显示每条分配记录，每行一个学生，包含宿舍号和匹配分等信息。

---

## 🟡 P1 — 中等缺陷（功能缺失或体验差）

### Bug 10: 管理员无删除任务功能

**问题**: 
- 后端没有删除任务的端点
- 前端任务表格没有"删除"按钮
- 管理员无法清理错误创建的任务

**修复方案**: 
1. 后端新增 `DELETE /api/admin/allocation/task/:taskId` 端点
2. 前端添加 `deleteTask(taskId)` API 函数和删除按钮（带确认弹窗）

---

### Bug 11: 管理员无用户管理页面

**问题**:
- 后端已实现 `GET /api/admin/users` 和 `DELETE /api/admin/users/:userId`
- 前端 `api/index.js` 已定义 `listUsers()` 和 `deleteUser()` 函数
- 但没有任何 Vue 页面调用这些函数，用户列表和删除功能完全不可用

**修复方案**: 在 AdminDashboard 中添加用户管理区域，或创建独立的用户管理页面。

---

### Bug 12: 无批量导入学生 UI

**问题**:
- 后端已实现 `POST /api/admin/students/import`
- 前端 `api/index.js` 已定义 `importStudents()` 函数
- 但 AdminDashboard 中没有上传按钮或导入界面

**修复方案**: 在管理后台添加"导入学生"按钮，支持 Excel/CSV 文件上传。

---

### Bug 13: 无分配规则配置 UI

**问题**:
- 后端已实现 `POST /api/admin/allocation/rule/save`
- 前端 `api/index.js` 已定义 `saveAllocationRule()` 函数
- 但没有规则配置的 UI 页面

**修复方案**: 在管理后台添加分配规则配置区域。

---

### Bug 14: 无导出分配结果按钮

**问题**:
- 后端已实现 `GET /api/admin/allocation/task/export/:taskId`
- 前端 `api/index.js` 已定义 `exportTaskResult()` 函数
- 但 AdminDashboard 的任务表格中没有"导出"按钮

**修复方案**: 在任务操作列添加"导出结果"按钮。

---

## 🟢 P2 — 轻微缺陷（可用但需优化）

### Bug 15: AdminAdjust 数据对接不完善

**文件**: `frontend-matching/src/views/AdminAdjust.vue`

**问题**: 
- 拖拽功能存在但没有宿舍选择器
- 数据从 `getTaskResult` 加载，但结果格式是按学生列出的，没有按宿舍分组
- 调整后无保存到后端的操作

---

### Bug 16: 后端多个端点无前端对接

| 后端端点 | 功能 | 前端状态 |
|---------|------|---------|
| `GET /api/student/questionnaire/template` | 获取问卷模板 | 无 API 函数，无调用 |
| `GET /api/student/questionnaire/status/:userId` | 查询问卷填写状态 | 无 API 函数，无调用 |
| `GET /api/student/info/:userId` | 获取学生信息 | 无 API 函数，无调用 |
| `GET /api/admin/allocation/rules` | 获取分配规则列表 | 无 API 函数，无调用 |
| `GET /api/admin/dashboard/stats` | 仪表盘统计数据 | 无 API 函数，无调用 |
| `POST /api/admin/transfer` | 管理员权限转让 | 无 API 函数，无调用 |

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
| P0 | Bug 6: 学生端无退出登录 | 学生无法退出 | 简单 | ❌ 未修复 |
| P0 | Bug 7: 返回大厅路由不存在 | 路由错误 | 简单 | ❌ 未修复 |
| P0 | Bug 8: 创建任务字段不匹配 | 后端忽略权重值 | 中等 | ❌ 未修复 |
| P0 | Bug 9: 结果表格列定义错误 | 分配学生列为空 | 中等 | ❌ 未修复 |
| P1 | Bug 10: 无删除任务功能 | 管理功能不完整 | 中等（需前后端） | ❌ 未修复 |
| P1 | Bug 11: 无用户管理页面 | 管理功能不完整 | 中等 | ❌ 未修复 |
| P1 | Bug 12: 无导入学生 UI | 管理功能不完整 | 中等 | ❌ 未修复 |
| P1 | Bug 13: 无规则配置 UI | 管理功能不完整 | 中等 | ❌ 未修复 |
| P1 | Bug 14: 无导出结果按钮 | 管理功能不完整 | 简单 | ❌ 未修复 |
| P2 | Bug 15: AdminAdjust 对接 | 调整功能受限 | 中等 | ❌ 未修复 |
| P2 | Bug 16: 后端端点无前端 | 功能缺失 | 中等 | ❌ 未修复 |
| P2 | Bug 17: 空状态提示 | 用户体验 | 简单 | ❌ 未修复 |

---

## 📝 修改记录

- 2026-05-25: 初始版本，记录 Bug 1-7
- 2026-05-26: Bug 1-5 标记为已修复；新增 Bug 6-17（全面 UI 缺陷检查）；移除"学生端无注册入口"（确认 Login.vue 已有注册表单）