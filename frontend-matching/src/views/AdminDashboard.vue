<template>
  <div class="admin-container">
    <div class="header-box">
      <h2>📊 管理员控制台 - 宿舍分配系统</h2>
      <el-button type="danger" plain @click="handleLogout">退出登录</el-button>
    </div>

    <!-- Statistics cards -->
    <el-row :gutter="20">
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="已导入学生总数" :value="stats.totalStudents" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="空闲宿舍床位" :value="stats.freeBeds" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="分配任务数" :value="stats.totalTasks" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="已完成任务数" :value="stats.completedTasks" />
        </el-card>
      </el-col>
    </el-row>

    <!-- Task list -->
    <el-card class="mt-20">
      <template #header>
        <div class="card-header">
          <span>🚀 分配任务列表</span>
          <el-button type="primary" @click="showCreateDialog = true">创建新分配任务</el-button>
        </div>
      </template>

      <div v-if="loadingTasks" style="text-align: center; padding: 40px;">
        <el-icon class="is-loading" :size="30"><Loading /></el-icon>
        <p style="color: #909399; margin-top: 10px;">加载任务列表...</p>
      </div>

      <el-empty v-else-if="tasks.length === 0" description="暂无分配任务" />

      <el-table v-else :data="tasks" style="width: 100%">
        <el-table-column prop="taskId" label="任务编号" width="200" />
        <el-table-column prop="taskName" label="任务名称" width="180" />
        <el-table-column prop="status" label="当前状态" width="150">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">{{ scope.row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="480">
          <template #default="scope">
            <el-button size="small" type="success" plain @click="handleRunTask(scope.row)"
              :loading="scope.row.running" :disabled="scope.row.status === 'completed'">
              执行匹配
            </el-button>
            <el-button size="small" type="primary" plain @click="handleViewResult(scope.row)"
              :disabled="scope.row.status !== 'completed'">
              查看结果
            </el-button>
            <el-button size="small" type="warning" plain @click="handleAdjust(scope.row)"
              :disabled="scope.row.status !== 'completed'">
              人工调整
            </el-button>
            <el-button size="small" type="info" plain @click="handleExport(scope.row)"
              :disabled="scope.row.status !== 'completed'">
              导出CSV
            </el-button>
            <el-button size="small" type="danger" plain @click="handleDeleteTask(scope.row)">
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Import students & User management row -->
    <el-row :gutter="20" class="mt-20">
      <!-- Import students card -->
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>📥 批量导入学生</span>
              <el-button type="primary" size="small" @click="showImportDialog = true">导入学生</el-button>
            </div>
          </template>
          <el-empty v-if="!importResult" description="点击上方按钮导入学生数据" :image-size="60" />
          <el-result v-else :icon="importResult.success ? 'success' : 'error'" :title="importResult.message" />
        </el-card>
      </el-col>

      <!-- User management card -->
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>👥 用户管理 ({{ userList.length }})</span>
              <el-button type="primary" size="small" @click="loadUserList">刷新</el-button>
            </div>
          </template>
          <el-table :data="userList.slice(0, 10)" style="width: 100%" size="small" max-height="250">
            <el-table-column prop="studentNo" label="学号" width="120" />
            <el-table-column prop="college" label="学院" width="120" show-overflow-tooltip />
            <el-table-column prop="role" label="角色" width="80">
              <template #default="scope">
                <el-tag :type="scope.row.role === 'admin' ? 'danger' : 'info'" size="small">
                  {{ scope.row.role === 'admin' ? '管理员' : '学生' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="150">
              <template #default="scope">
                <el-button v-if="scope.row.role !== 'admin'" size="small" type="danger" text
                  @click="handleDeleteUser(scope.row)">删除</el-button>
                <el-button v-if="scope.row.role !== 'admin'" size="small" type="warning" text
                  @click="handleTransferAdmin(scope.row)">设为管理员</el-button>
              </template>
            </el-table-column>
          </el-table>
          <div v-if="userList.length > 10" style="text-align: center; padding: 8px; color: #909399; font-size: 12px;">
            仅显示前 10 条，共 {{ userList.length }} 条记录
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Import students dialog -->
    <el-dialog v-model="showImportDialog" title="批量导入学生" width="600px">
      <el-alert type="info" :closable="false" style="margin-bottom: 16px;">
        请粘贴 JSON 数组格式的学生数据。每个学生包含：studentNo（必填）、gender、college、major、grade、password（默认123456）。
      </el-alert>
      <el-input v-model="importJson" type="textarea" :rows="10"
        placeholder='[{"studentNo":"2024001","gender":"male","college":"计算机学院","major":"软件工程","grade":"2024"}]' />
      <template #footer>
        <el-button @click="showImportDialog = false">取消</el-button>
        <el-button type="primary" @click="handleImportStudents" :loading="importing">确认导入</el-button>
      </template>
    </el-dialog>

    <!-- Create task dialog -->
    <el-dialog v-model="showCreateDialog" title="创建分配任务" width="500px">
      <el-form label-position="top">
        <el-form-item label="任务名称">
          <el-input v-model="newTask.taskName" placeholder="如：计算机学院2026级分配" />
        </el-form-item>
        <el-form-item label="目标学院">
          <el-input v-model="newTask.college" placeholder="如：计算机学院" />
        </el-form-item>
        <el-form-item label="目标专业">
          <el-input v-model="newTask.major" placeholder="如：软件工程（留空表示全部专业）" />
        </el-form-item>
        <el-form-item label="性别限制">
          <el-select v-model="newTask.gender" placeholder="请选择" style="width: 100%;">
            <el-option label="不限" value="" />
            <el-option label="男" value="male" />
            <el-option label="女" value="female" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreateTask" :loading="creating">创建</el-button>
      </template>
    </el-dialog>

    <!-- Task result dialog — displays flat per-student allocation rows from backend -->
    <el-dialog v-model="showResultDialog" :title="'分配结果 — ' + resultTaskName" width="800px">
      <el-table :data="taskResults" style="width: 100%" max-height="400">
        <el-table-column prop="studentNo" label="学号" width="140" />
        <el-table-column prop="building" label="楼栋" width="100" />
        <el-table-column prop="roomNumber" label="房间号" width="100" />
        <el-table-column prop="totalScore" label="匹配分" width="100">
          <template #default="scope">
            {{ (scope.row.totalScore * 100).toFixed(1) }}
          </template>
        </el-table-column>
        <el-table-column prop="explanationText" label="匹配说明" show-overflow-tooltip />
      </el-table>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { listTasks, createTask, runTask, getTaskResult, listUsers, listDormitories, exportTaskResult, deleteTask, importStudents, deleteUser, transferAdmin } from '../api'

const router = useRouter()
const loadingTasks = ref(false)
const creating = ref(false)
const showCreateDialog = ref(false)
const showResultDialog = ref(false)
const showImportDialog = ref(false)
const resultTaskName = ref('')
const tasks = ref([])
const taskResults = ref([])
const userList = ref([])
const importJson = ref('')
const importing = ref(false)
const importResult = ref(null)

const stats = reactive({
  totalStudents: 0,
  freeBeds: 0,
  totalTasks: 0,
  completedTasks: 0
})

const newTask = reactive({
  taskName: '',
  college: '',
  major: '',
  gender: ''
})

const getStatusType = (status) => {
  if (status === 'completed') return 'success'
  if (status === 'running') return 'warning'
  if (status === 'failed') return 'danger'
  return 'info'
}

// Load task list from backend
const loadTasks = async () => {
  loadingTasks.value = true
  try {
    const data = await listTasks()
    // Backend returns { tasks: [...] }, interceptor unwraps to { tasks: [...] }
    const taskList = Array.isArray(data?.tasks) ? data.tasks : (Array.isArray(data) ? data : [])
    tasks.value = taskList.map(t => ({ ...t, running: false }))
    stats.totalTasks = taskList.length
    stats.completedTasks = taskList.filter(t => t.status === 'completed').length
  } catch (err) {
    console.error('Load tasks failed:', err)
    // If API fails, show empty state (not mock data)
  } finally {
    loadingTasks.value = false
  }
}

// Create a new task — sends taskName, college, major, gender to match backend expectations
const handleCreateTask = async () => {
  if (!newTask.taskName) {
    return ElMessage.warning('请输入任务名称')
  }
  creating.value = true
  try {
    await createTask({
      taskName: newTask.taskName,
      college: newTask.college,
      major: newTask.major,
      gender: newTask.gender
    })
    ElMessage.success('任务创建成功')
    showCreateDialog.value = false
    newTask.taskName = ''
    newTask.college = ''
    newTask.major = ''
    newTask.gender = ''
    await loadTasks()
  } catch (err) {
    console.error('Create task failed:', err)
  } finally {
    creating.value = false
  }
}

// Run matching algorithm
const handleRunTask = async (task) => {
  task.running = true
  try {
    await runTask(task.taskId)
    ElMessage.success('匹配算法执行完成')
    await loadTasks()
  } catch (err) {
    console.error('Run task failed:', err)
  } finally {
    task.running = false
  }
}

// View task result — backend returns flat per-student rows in data.allocations
const handleViewResult = async (task) => {
  try {
    const data = await getTaskResult(task.taskId)
    resultTaskName.value = data?.taskName || task.taskName || ''
    taskResults.value = Array.isArray(data?.allocations) ? data.allocations : (Array.isArray(data) ? data : [])
    showResultDialog.value = true
  } catch (err) {
    console.error('Get task result failed:', err)
  }
}

// Load dashboard statistics: total students and free beds
const loadDashboardStats = async () => {
  try {
    const [usersData, dormsData] = await Promise.all([listUsers(), listDormitories()])
    const userList = Array.isArray(usersData?.users) ? usersData.users : (Array.isArray(usersData) ? usersData : [])
    const dormList = Array.isArray(dormsData?.dormitories) ? dormsData.dormitories : (Array.isArray(dormsData) ? dormsData : [])

    // Count students (non-admin users)
    stats.totalStudents = userList.filter(u => u.role !== 'admin').length

    // Sum up available capacity from all dormitories
    stats.freeBeds = dormList.reduce((sum, d) => sum + (d.capacity || 0), 0)
  } catch (err) {
    console.error('Load dashboard stats failed:', err)
    // Stats will remain 0 if API fails — acceptable fallback
  }
}

// Export task result as CSV
const handleExport = async (task) => {
  try {
    const csv = await exportTaskResult(task.taskId)
    // Create downloadable blob from CSV string
    const blob = new Blob([typeof csv === 'string' ? csv : JSON.stringify(csv)], { type: 'text/csv;charset=utf-8;' })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = `${task.taskName || task.taskId}_result.csv`
    link.click()
    URL.revokeObjectURL(url)
    ElMessage.success('导出成功')
  } catch (err) {
    console.error('Export failed:', err)
  }
}

// Delete a task with confirmation
const handleDeleteTask = async (task) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除任务「${task.taskName || task.taskId}」吗？删除后相关分配结果也将被清除。`,
      '确认删除',
      { confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning' }
    )
    await deleteTask(task.taskId)
    ElMessage.success('任务已删除')
    await loadTasks()
  } catch (err) {
    if (err !== 'cancel') {
      console.error('Delete task failed:', err)
    }
  }
}

// Navigate to adjust page
const handleAdjust = (task) => {
  router.push({ path: '/admin/adjust', query: { taskId: task.taskId } })
}

// Load user list for user management section
const loadUserList = async () => {
  try {
    const data = await listUsers()
    userList.value = Array.isArray(data?.users) ? data.users : (Array.isArray(data) ? data : [])
  } catch (err) {
    console.error('Load users failed:', err)
  }
}

// Import students from JSON
const handleImportStudents = async () => {
  if (!importJson.value.trim()) {
    return ElMessage.warning('请粘贴学生数据')
  }
  try {
    const parsed = JSON.parse(importJson.value)
    importing.value = true
    const data = await importStudents(parsed)
    const count = data?.imported ?? 0
    importResult.value = { success: true, message: `成功导入 ${count} 名学生` }
    ElMessage.success(`成功导入 ${count} 名学生`)
    showImportDialog.value = false
    importJson.value = ''
    await loadDashboardStats()
    await loadUserList()
  } catch (err) {
    importResult.value = { success: false, message: '导入失败: ' + (err.message || '请检查JSON格式') }
    console.error('Import students failed:', err)
  } finally {
    importing.value = false
  }
}

// Delete a user with confirmation
const handleDeleteUser = async (user) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除用户「${user.studentNo}」吗？该操作不可恢复。`,
      '确认删除用户',
      { confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning' }
    )
    await deleteUser(user.userId)
    ElMessage.success('用户已删除')
    await loadUserList()
    await loadDashboardStats()
  } catch (err) {
    if (err !== 'cancel') {
      console.error('Delete user failed:', err)
    }
  }
}

// Transfer admin role to another user
const handleTransferAdmin = async (user) => {
  try {
    await ElMessageBox.confirm(
      `确定要将管理员权限转让给「${user.studentNo}」吗？转让后您将变为普通学生，需要重新登录。`,
      '确认权限转让',
      { confirmButtonText: '确定转让', cancelButtonText: '取消', type: 'warning' }
    )
    await transferAdmin(user.userId)
    ElMessage.success('管理员权限已转让，请重新登录')
    handleLogout()
  } catch (err) {
    if (err !== 'cancel') {
      console.error('Transfer admin failed:', err)
    }
  }
}

// Logout
const handleLogout = () => {
  localStorage.removeItem('token')
  localStorage.removeItem('userId')
  localStorage.removeItem('role')
  router.push('/login')
}

onMounted(() => {
  const role = localStorage.getItem('role')
  if (role !== 'admin') {
    ElMessage.error('请以管理员身份登录')
    router.push('/login')
    return
  }
  loadTasks()
  loadDashboardStats()
  loadUserList()
})
</script>

<style scoped>
.admin-container { padding: 30px; background-color: #f0f2f5; min-height: 100vh; }
.header-box { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;}
.header-box h2 { margin: 0; color: #303133; }
.mt-20 { margin-top: 20px; }
.card-header { display: flex; justify-content: space-between; align-items: center; }
</style>