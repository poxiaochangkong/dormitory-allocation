<![CDATA[<!-- AdminAdjust.vue -->
<!-- 
  管理员控制台 - 人工微调
  - 按宿舍分组展示分配结果
  - 选择两个学生交换宿舍
  - 调用后端 adjustDormitory API
-->
<template>
  <div class="adjust-container">
    <el-card class="box-card">
      <template #header>
        <div class="header-box">
          <h2>🛠️ 人工微调 — 交换宿舍分配</h2>
          <el-button type="primary" plain @click="$router.push('/admin/dashboard')">返回数据大盘</el-button>
        </div>
        <p class="subtitle">选择两名学生，点击"交换"即可互换他们的宿舍分配结果。</p>
      </template>

      <!-- Loading state -->
      <div v-if="loading" style="text-align: center; padding: 60px 0;">
        <el-icon class="is-loading" :size="30"><Loading /></el-icon>
        <p style="color: #909399; margin-top: 10px;">加载分配数据...</p>
      </div>

      <!-- No task ID -->
      <el-empty v-else-if="!taskId" description="未指定任务，请从管理面板进入">
        <el-button type="primary" @click="$router.push('/admin/dashboard')">返回管理面板</el-button>
      </el-empty>

      <template v-else>
        <!-- Swap controls -->
        <el-card shadow="never" class="swap-panel">
          <h3>🔄 交换操作</h3>
          <el-row :gutter="20" align="middle">
            <el-col :span="9">
              <div class="swap-box" :class="{ selected: selected1 }">
                <p class="swap-label">学生 A</p>
                <template v-if="selected1">
                  <p class="swap-info">{{ selected1.studentNo }}</p>
                  <p class="swap-dorm">{{ selected1.building }} {{ selected1.roomNumber }}</p>
                </template>
                <p v-else class="swap-placeholder">点击下方表格选择</p>
              </div>
            </el-col>
            <el-col :span="6" style="text-align: center;">
              <el-button
                type="warning"
                size="large"
                :disabled="!selected1 || !selected2 || swapping"
                :loading="swapping"
                @click="handleSwap"
              >
                ⇄ 交换宿舍
              </el-button>
            </el-col>
            <el-col :span="9">
              <div class="swap-box" :class="{ selected: selected2 }">
                <p class="swap-label">学生 B</p>
                <template v-if="selected2">
                  <p class="swap-info">{{ selected2.studentNo }}</p>
                  <p class="swap-dorm">{{ selected2.building }} {{ selected2.roomNumber }}</p>
                </template>
                <p v-else class="swap-placeholder">点击下方表格选择</p>
              </div>
            </el-col>
          </el-row>
        </el-card>

        <!-- Allocation results grouped by dorm -->
        <div class="dorm-groups">
          <el-card
            v-for="group in dormGroups"
            :key="group.dormId"
            shadow="hover"
            class="dorm-card"
          >
            <template #header>
              <div class="dorm-header">
                <span>{{ group.building }} {{ group.roomNumber }}</span>
                <el-tag type="success" size="small">{{ group.students.length }} 人</el-tag>
              </div>
            </template>

            <el-table :data="group.students" size="small" @row-click="handleRowClick">
              <el-table-column prop="studentNo" label="学号" width="140" />
              <el-table-column prop="totalScore" label="匹配分" width="100">
                <template #default="scope">
                  {{ (scope.row.totalScore * 100).toFixed(1) }}
                </template>
              </el-table-column>
              <el-table-column prop="explanationText" label="匹配说明" show-overflow-tooltip />
              <el-table-column width="80" align="center">
                <template #default="scope">
                  <el-tag
                    v-if="isSelected(scope.row)"
                    type="warning"
                    size="small"
                  >
                    {{ getSelectedLabel(scope.row) }}
                  </el-tag>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </div>

        <!-- Empty state when no results -->
        <el-empty v-if="dormGroups.length === 0" description="该任务暂无分配结果" />
      </template>
    </el-card>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ElMessage, ElNotification } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { getTaskResult, adjustDormitory } from '../api'

const router = useRouter()
const route = useRoute()
const loading = ref(true)
const swapping = ref(false)
const taskId = ref('')
const allocations = ref([])

// Two selected students for swapping
const selected1 = ref(null)
const selected2 = ref(null)

// Group allocations by dorm
const dormGroups = computed(() => {
  const map = new Map()
  for (const item of allocations.value) {
    const key = item.dormId || 'unassigned'
    if (!map.has(key)) {
      map.set(key, {
        dormId: key,
        building: item.building || '-',
        roomNumber: item.roomNumber || '-',
        students: []
      })
    }
    map.get(key).students.push(item)
  }
  return Array.from(map.values())
})

// Check if a row is selected
const isSelected = (row) => {
  return (selected1.value && selected1.value.userId === row.userId) ||
         (selected2.value && selected2.value.userId === row.userId)
}

const getSelectedLabel = (row) => {
  if (selected1.value && selected1.value.userId === row.userId) return 'A'
  if (selected2.value && selected2.value.userId === row.userId) return 'B'
  return ''
}

// Handle row click — alternate between selecting student A and B
const handleRowClick = (row) => {
  if (selected1.value && selected1.value.userId === row.userId) {
    selected1.value = null
    return
  }
  if (selected2.value && selected2.value.userId === row.userId) {
    selected2.value = null
    return
  }

  if (!selected1.value) {
    selected1.value = row
  } else if (!selected2.value) {
    selected2.value = row
  } else {
    // Both selected — replace student A
    selected1.value = selected2.value
    selected2.value = row
  }
}

// Execute swap via backend API
const handleSwap = async () => {
  swapping.value = true
  try {
    await adjustDormitory({
      taskId: taskId.value,
      userId1: selected1.value.userId,
      userId2: selected2.value.userId
    })

    ElNotification({
      title: '交换成功',
      message: `${selected1.value.studentNo} ↔ ${selected2.value.studentNo} 宿舍已互换`,
      type: 'success',
      duration: 3000
    })

    // Clear selection and reload data
    selected1.value = null
    selected2.value = null
    await loadData()
  } catch (err) {
    console.error('Swap failed:', err)
  } finally {
    swapping.value = false
  }
}

// Load task result data from backend
const loadData = async () => {
  taskId.value = route.query.taskId || ''
  if (!taskId.value) {
    loading.value = false
    return
  }

  loading.value = true
  try {
    const data = await getTaskResult(taskId.value)
    allocations.value = Array.isArray(data?.allocations)
      ? data.allocations
      : (Array.isArray(data) ? data : [])
  } catch (err) {
    console.error('Load task result failed:', err)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  const role = localStorage.getItem('role')
  if (role !== 'admin') {
    ElMessage.error('请以管理员身份登录')
    router.push('/login')
    return
  }
  loadData()
})
</script>

<style scoped>
.adjust-container { padding: 30px; display: flex; justify-content: center; background-color: #f5f7fa; min-height: 100vh; }
.box-card { width: 1100px; }
.header-box { display: flex; justify-content: space-between; align-items: center; }
.header-box h2 { margin: 0; color: #303133; }
.subtitle { color: #909399; font-size: 14px; margin-top: 5px; }

/* Swap panel */
.swap-panel { margin-bottom: 25px; background: #fafafa; }
.swap-panel h3 { margin: 0 0 15px 0; color: #E6A23C; }
.swap-box { text-align: center; padding: 15px; border: 2px dashed #dcdfe6; border-radius: 8px; min-height: 90px; transition: all 0.3s; }
.swap-box.selected { border-color: #E6A23C; background: #fdf6ec; }
.swap-label { font-size: 14px; color: #909399; margin: 0 0 8px 0; }
.swap-info { font-size: 16px; font-weight: bold; color: #303133; margin: 0; }
.swap-dorm { font-size: 14px; color: #67C23A; margin: 5px 0 0 0; }
.swap-placeholder { color: #c0c4cc; margin: 20px 0 0 0; }

/* Dorm groups */
.dorm-groups { display: flex; flex-direction: column; gap: 20px; }
.dorm-card { cursor: default; }
.dorm-header { display: flex; justify-content: space-between; align-items: center; font-weight: bold; }

/* Row click cursor */
:deep(.el-table__row) { cursor: pointer; }
</style>
]]>