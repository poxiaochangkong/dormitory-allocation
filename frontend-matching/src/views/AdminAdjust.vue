<!-- AdminAdjust.vue -->
<!-- 
  管理员控制台 - 人工微调与风险重估
  - 拖拽学生卡片调整分配
  - 调用后端 API 提交调整结果
  - 实时显示一票否决风险预警
-->
<template>
  <div class="adjust-container">
    <el-card class="box-card">
      <template #header>
        <div class="header-box">
          <h2>🛠️ 管理员控制台 - 人工微调与风险重估</h2>
          <el-button type="primary" plain @click="$router.push('/admin/dashboard')">返回数据大盘</el-button>
        </div>
        <p class="subtitle">拖拽学生卡片进行强制调宿，系统将实时计算违规风险。</p>
      </template>

      <div v-if="loading" style="text-align: center; padding: 60px 0;">
        <el-icon class="is-loading" :size="30"><Loading /></el-icon>
        <p style="color: #909399; margin-top: 10px;">加载分配数据...</p>
      </div>

      <template v-else>
        <el-row :gutter="30">
          <el-col :span="10">
            <div class="pool-header">
              <h3>📋 待处理学生池</h3>
              <el-tag type="info">{{ unassigned.length }} 人</el-tag>
            </div>
            <draggable
              v-model="unassigned"
              group="students"
              item-key="id"
              class="drag-area"
              animation="200"
            >
              <template #item="{ element }">
                <div class="student-card warning-card">
                  <div class="card-info">
                    <strong>{{ element.name || element.studentNo }}</strong>
                    <span v-if="element.major"> ({{ element.major }})</span>
                    <p v-if="element.trait" class="trait-tag">⚠️ {{ element.trait }}</p>
                  </div>
                </div>
              </template>
            </draggable>
          </el-col>

          <el-col :span="10">
            <div class="pool-header">
              <h3>🏠 目标宿舍：{{ currentDormLabel }}</h3>
              <el-tag type="success">当前 {{ dormStudents.length }} 人</el-tag>
            </div>
            <draggable
              v-model="dormStudents"
              group="students"
              item-key="id"
              class="drag-area dorm-area"
              animation="200"
              @add="onAddStudent"
            >
              <template #item="{ element }">
                <div class="student-card safe-card">
                  <div class="card-info">
                    <strong>{{ element.name || element.studentNo }}</strong>
                    <span v-if="element.major"> ({{ element.major }})</span>
                    <p v-if="element.trait" class="trait-tag">✅ {{ element.trait }}</p>
                  </div>
                </div>
              </template>
            </draggable>
          </el-col>
        </el-row>

        <div style="text-align: center; margin-top: 20px;">
          <el-button type="success" size="large" @click="submitAdjust" :loading="submitting">
            保存调整结果
          </el-button>
        </div>
      </template>
    </el-card>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ElMessage, ElNotification } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import draggable from 'vuedraggable'
import { getTaskResult, adjustDormitory } from '../api'

const router = useRouter()
const route = useRoute()
const loading = ref(true)
const submitting = ref(false)
const taskId = ref('')
const currentDormLabel = ref('选择宿舍')

// Students to be assigned
const unassigned = ref([])
// Students already in the target dorm
const dormStudents = ref([])

// Load task result data
const loadData = async () => {
  taskId.value = route.query.taskId || ''
  if (!taskId.value) {
    ElMessage.warning('未指定任务，请从管理面板进入')
    loading.value = false
    return
  }

  try {
    const data = await getTaskResult(taskId.value)
    const results = Array.isArray(data) ? data : (data ? [data] : [])

    // Populate unassigned pool with result data
    if (results.length > 0) {
      const allStudents = []
      results.forEach(r => {
        if (r.userIds && Array.isArray(r.userIds)) {
          r.userIds.forEach(uid => {
            allStudents.push({
              id: uid,
              userId: uid,
              studentNo: uid,
              name: uid,
              major: '',
              trait: '',
              originalDorm: r.dormId
            })
          })
        }
      })
      unassigned.value = allStudents
      // Use first dorm as default target
      if (results[0] && results[0].dormId) {
        currentDormLabel.value = results[0].dormId
      }
    }
  } catch (err) {
    console.error('Load task result failed:', err)
  } finally {
    loading.value = false
  }
}

// Handle student added to dorm
const onAddStudent = (evt) => {
  const addedStudent = dormStudents.value[evt.newIndex]
  ElMessage.success(`已将学生调整至宿舍`)
}

// Submit adjustment to backend
const submitAdjust = async () => {
  submitting.value = true
  try {
    const assignments = dormStudents.value.map(s => ({
      userId: s.userId || s.id,
      dormId: currentDormLabel.value
    }))

    if (assignments.length === 0) {
      ElMessage.warning('请先将学生拖入宿舍')
      submitting.value = false
      return
    }

    await adjustDormitory({
      taskId: taskId.value,
      assignments: assignments
    })

    ElNotification({
      title: '调整成功',
      message: `已成功调整 ${assignments.length} 名学生的宿舍分配`,
      type: 'success',
      duration: 3000
    })
  } catch (err) {
    console.error('Adjust failed:', err)
  } finally {
    submitting.value = false
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
.box-card { width: 1000px; }
.header-box { display: flex; justify-content: space-between; align-items: center; }
.header-box h2 { margin: 0; color: #303133; }
.subtitle { color: #909399; font-size: 14px; margin-top: 5px; }

.pool-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; border-bottom: 2px solid #ebeef5; padding-bottom: 10px;}
.pool-header h3 { margin: 0; color: #409EFF; }

/* Drag areas */
.drag-area { min-height: 400px; padding: 15px; background: #fafafa; border-radius: 8px; border: 1px dashed #dcdfe6; }
.dorm-area { background: #f0f9eb; border: 1px dashed #e1f3d8; }

/* Card styles */
.student-card { padding: 15px; margin-bottom: 15px; background: white; border-radius: 6px; cursor: grab; transition: all 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.05); }
.student-card:active { cursor: grabbing; transform: scale(1.02); box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
.warning-card { border-left: 5px solid #F56C6C; }
.safe-card { border-left: 5px solid #67C23A; }

.card-info { font-size: 15px; }
.trait-tag { margin: 8px 0 0 0; font-size: 13px; color: #909399; background: #f4f4f5; padding: 4px 8px; border-radius: 4px; display: inline-block;}
</style>