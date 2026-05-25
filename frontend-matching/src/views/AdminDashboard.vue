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
        <el-table-column prop="name" label="任务名称" width="180" />
        <el-table-column prop="status" label="当前状态" width="150">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">{{ scope.row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="350">
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
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Create task dialog -->
    <el-dialog v-model="showCreateDialog" title="创建分配任务" width="500px">
      <el-form label-position="top">
        <el-form-item label="任务名称">
          <el-input v-model="newTask.name" placeholder="如：计算机学院2026级分配" />
        </el-form-item>
        <el-form-item label="目标学院">
          <el-input v-model="newTask.college" placeholder="如：计算机学院" />
        </el-form-item>
        <el-form-item label="匹配算法权重">
          <el-row :gutter="10">
            <el-col :span="8">
              <el-input v-model.number="newTask.similarityWeight" placeholder="相似度">
                <template #prepend>相似</template>
              </el-input>
            </el-col>
            <el-col :span="8">
              <el-input v-model.number="newTask.complementarityWeight" placeholder="互补度">
                <template #prepend>互补</template>
              </el-input>
            </el-col>
            <el-col :span="8">
              <el-input v-model.number="newTask.vetoSafetyWeight" placeholder="安全度">
                <template #prepend>安全</template>
              </el-input>
            </el-col>
          </el-row>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreateTask" :loading="creating">创建</el-button>
      </template>
    </el-dialog>

    <!-- Task result dialog -->
    <el-dialog v-model="showResultDialog" title="分配结果" width="700px">
      <el-table :data="taskResults" style="width: 100%" max-height="400">
        <el-table-column prop="dormId" label="宿舍" width="150" />
        <el-table-column prop="userIds" label="分配学生" />
        <el-table-column prop="totalScore" label="匹配分" width="100">
          <template #default="scope">
            {{ (scope.row.totalScore * 100).toFixed(1) }}
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { listTasks, createTask, runTask, getTaskResult } from '../api'

const router = useRouter()
const loadingTasks = ref(false)
const creating = ref(false)
const showCreateDialog = ref(false)
const showResultDialog = ref(false)
const tasks = ref([])
const taskResults = ref([])

const stats = reactive({
  totalStudents: 0,
  freeBeds: 0,
  totalTasks: 0,
  completedTasks: 0
})

const newTask = reactive({
  name: '',
  college: '',
  similarityWeight: 0.5,
  complementarityWeight: 0.25,
  vetoSafetyWeight: 0.25
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
    if (Array.isArray(data)) {
      tasks.value = data.map(t => ({ ...t, running: false }))
      stats.totalTasks = data.length
      stats.completedTasks = data.filter(t => t.status === 'completed').length
    }
  } catch (err) {
    console.error('Load tasks failed:', err)
    // If API fails, show empty state (not mock data)
  } finally {
    loadingTasks.value = false
  }
}

// Create a new task
const handleCreateTask = async () => {
  if (!newTask.name) {
    return ElMessage.warning('请输入任务名称')
  }
  creating.value = true
  try {
    await createTask({
      name: newTask.name,
      college: newTask.college,
      similarityWeight: newTask.similarityWeight,
      complementarityWeight: newTask.complementarityWeight,
      vetoSafetyWeight: newTask.vetoSafetyWeight
    })
    ElMessage.success('任务创建成功')
    showCreateDialog.value = false
    newTask.name = ''
    newTask.college = ''
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

// View task result
const handleViewResult = async (task) => {
  try {
    const data = await getTaskResult(task.taskId)
    taskResults.value = Array.isArray(data) ? data : (data ? [data] : [])
    showResultDialog.value = true
  } catch (err) {
    console.error('Get task result failed:', err)
  }
}

// Navigate to adjust page
const handleAdjust = (task) => {
  router.push({ path: '/admin/adjust', query: { taskId: task.taskId } })
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
})
</script>

<style scoped>
.admin-container { padding: 30px; background-color: #f0f2f5; min-height: 100vh; }
.header-box { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;}
.header-box h2 { margin: 0; color: #303133; }
.mt-20 { margin-top: 20px; }
.card-header { display: flex; justify-content: space-between; align-items: center; }
</style>