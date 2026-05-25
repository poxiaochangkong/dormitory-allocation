# Bug Report - 前后端对接问题汇总

> 生成时间：2026-05-25
> 范围：前端 `frontend-matching` 与 后端 `backend` 的 API 对接问题

---

## 🔴 严重问题（功能完全不可用）

### Bug 1: AdminDashboard 任务列表不显示

**文件**: `frontend-matching/src/views/AdminDashboard.vue`

**原因**: `loadTasks()` 中数据格式不匹配

后端 `ListTasks` 返回格式：
```json
{ "tasks": [{ "taskId": "...", "taskName": "...", "status": "..." }] }
```

经过 `request.js` 拦截器解包后，`data` 是对象 `{ tasks: [...] }`

但前端代码写的是：
```js
const data = await listTasks()
if (Array.isArray(data)) {  // ← 永远为 false！data 是对象不是数组
    tasks.value = data.map(...)
}
```

**修复方案**: 改为 `if (Array.isArray(data?.tasks))` 并从 `data.tasks` 读取

---

### Bug 2: 创建任务时任务名称丢失

**文件**: `frontend-matching/src/views/AdminDashboard.vue`

**原因**: 前后端字段名不一致

前端发送：
```js
{ name: newTask.name, college: newTask.college, ... }
```

后端读取：
```cpp
std::string task_name = data.value("taskName", "Unnamed Task");
```

前端用 `name`，后端用 `taskName`，导致任务名称永远为 "Unnamed Task"

**修复方案**: 前端改为发送 `taskName` 字段

---

### Bug 3: 任务表格"任务名称"列不显示

**文件**: `frontend-matching/src/views/AdminDashboard.vue`

**原因**: 表格列 prop 与后端返回字段不匹配

前端：
```html
<el-table-column prop="name" label="任务名称" />
```

后端返回的字段名是 `taskName`，不是 `name`

**修复方案**: 改为 `prop="taskName"`

---

## 🟡 中等问题（功能缺失）

### Bug 4: 前端有 API 函数但无页面调用

以下 API 函数已在 `api/index.js` 中定义，但没有被任何 Vue 页面使用：

| API 函数 | 后端端点 | 缺少的页面 |
|---------|---------|-----------|
| `importStudents()` | `POST /api/admin/students/import` | 批量导入学生页面 |
| `exportTaskResult()` | `GET /api/admin/allocation/task/export/:taskId` | AdminDashboard 缺少导出按钮 |
| `listUsers()` | `GET /api/admin/users` | 用户管理页面 |
| `listDormitories()` / `addDormitory()` | `GET/POST /api/admin/dormitories` | 宿舍管理页面 |
| `saveAllocationRule()` | `POST /api/admin/allocation/rule/save` | 规则配置页面 |

---

### Bug 5: 后端有端点但前端完全没有对应功能

| 后端端点 | 功能 | 前端状态 |
|---------|------|---------|
| `DELETE /api/admin/users/:userId` | 删除用户 | 没有 API 函数，没有页面 |
| `POST /api/admin/transfer` | 管理员权限转让 | 没有 API 函数，没有页面 |
| `GET /api/student/questionnaire/template` | 获取问卷模板 | 没有 API 函数，没有页面 |

---

## 🟢 轻微问题

### Bug 6: AdminAdjust 宿舍选择功能不完善

**文件**: `frontend-matching/src/views/AdminAdjust.vue`

- 拖拽功能存在但没有宿舍选择器，只能默认使用第一个宿舍
- 数据从 `getTaskResult` 加载，但结果格式是按学生列出的，没有按宿舍分组

### Bug 7: StudentResult 空状态处理

**文件**: `frontend-matching/src/views/StudentResult.vue`

- 管理员未运行分配任务时，只能显示"暂无分配结果"的空状态
- 缺少引导用户返回完成问卷的提示

---

## 📋 修复优先级

| 优先级 | Bug | 影响范围 | 修复难度 |
|-------|-----|---------|---------|
| P0 | Bug 1: 任务列表不显示 | 管理员无法使用系统 | 简单（改 1 行） |
| P0 | Bug 2: 任务名称丢失 | 数据展示错误 | 简单（改 1 行） |
| P0 | Bug 3: 表格列不匹配 | UI 显示问题 | 简单（改 1 行） |
| P1 | Bug 4: 缺少管理页面 | 管理功能不完整 | 中等（需新增页面） |
| P2 | Bug 5: 端点无前端 | 功能缺失 | 中等（需新增 API + 页面） |
| P2 | Bug 6: 宿舍选择器 | 调整功能受限 | 中等 |
| P3 | Bug 7: 空状态提示 | 用户体验 | 简单 |