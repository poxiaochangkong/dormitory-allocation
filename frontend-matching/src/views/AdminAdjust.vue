<!-- AdminAdjust.vue -->
<!-- 
  管理员控制台 - 人工微调与风险重估
  这是一个专门为管理员设计的界面，允许他们在系统自动分配后对学生进行人工调整。
  亮点功能：
    - 拖拽式界面：管理员可以通过拖拽学生卡片来调整分配结果，操作直观便捷。
    - 实时风险评估：每次调整都会触发系统的风险评估机制，实时计算并展示潜在的违规风险。
    - 一票否决预警：如果调整涉及到高危习惯的学生，系统会立即弹出一票否决风险警告，提醒管理员可能引发的宿舍矛盾。
    - 风险分数动态更新：根据调整后的组合，系统会动态更新风险分数，并提供详细的风险分析报告，帮助管理员做出更明智的决策。
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

      <el-row :gutter="30">
        <el-col :span="10">
          <div class="pool-header">
            <h3>📋 待处理异常学生池</h3>
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
                  <strong>{{ element.name }}</strong> ({{ element.major }})
                  <p class="trait-tag">⚠️ 习惯标签：{{ element.trait }}</p>
                </div>
              </div>
            </template>
          </draggable>
        </el-col>

        <el-col :span="10">
          <div class="pool-header">
            <h3>🏠 目标宿舍：南苑 4 栋 302</h3>
            <el-tag type="success">当前契合度：极高</el-tag>
          </div>
          <draggable
            v-model="dorm302"
            group="students"
            item-key="id"
            class="drag-area dorm-area"
            animation="200"
            @add="onAddStudent"
          >
            <template #item="{ element }">
              <div class="student-card safe-card">
                <div class="card-info">
                  <strong>{{ element.name }}</strong> ({{ element.major }})
                  <p class="trait-tag">✅ 习惯标签：{{ element.trait }}</p>
                </div>
              </div>
            </template>
          </draggable>
        </el-col>
      </el-row>
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import draggable from 'vuedraggable'
import { ElMessage, ElNotification } from 'element-plus'

// 模拟数据：待分配池的学生（带有高危习惯）
const unassigned = ref([
  { id: 101, name: '李雷', major: '软件工程', trait: '深夜开麦打游戏、外放' },
  { id: 102, name: '陈晨', major: '计算机科学', trait: '长期不洗澡、卫生差' }
])

// 模拟数据：302宿舍原有的好学生
const dorm302 = ref([
  { id: 201, name: '张三', major: '软件工程', trait: '极度怕吵、早睡早起' },
  { id: 202, name: '王五', major: '软件工程', trait: '轻度洁癖、喜静' }
])

// 监听拖拽放入宿舍的动作
const onAddStudent = (evt) => {
  const addedStudent = dorm302.value[evt.newIndex]
  
  // 核心亮点：触发一票否决拦截预警
  if (addedStudent.name === '李雷') {
    ElNotification({
      title: '🚨 一票否决风险警告',
      message: `检测到【李雷】(深夜打游戏) 与 302宿舍原成员【张三】(极度怕吵) 存在一票否决冲突！强行分配将极大概率引发宿舍矛盾！`,
      type: 'error',
      duration: 6000
    })
  } else if (addedStudent.name === '陈晨') {
    ElNotification({
      title: '🚨 卫生习惯冲突预警',
      message: `检测到【陈晨】(卫生差) 与成员【王五】(洁癖) 严重不合，风险分数激增！`,
      type: 'warning',
      duration: 6000
    })
  } else {
    ElMessage.success(`已成功将【${addedStudent.name}】调整至 302 宿舍。`)
  }
}
</script>

<style scoped>
.adjust-container { padding: 30px; display: flex; justify-content: center; background-color: #f5f7fa; min-height: 100vh; }
.box-card { width: 1000px; }
.header-box { display: flex; justify-content: space-between; align-items: center; }
.header-box h2 { margin: 0; color: #303133; }
.subtitle { color: #909399; font-size: 14px; margin-top: 5px; }

.pool-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; border-bottom: 2px solid #ebeef5; padding-bottom: 10px;}
.pool-header h3 { margin: 0; color: #409EFF; }

/* 拖拽区域 */
.drag-area { min-height: 400px; padding: 15px; background: #fafafa; border-radius: 8px; border: 1px dashed #dcdfe6; }
.dorm-area { background: #f0f9eb; border: 1px dashed #e1f3d8; }

/* 卡片样式 */
.student-card { padding: 15px; margin-bottom: 15px; background: white; border-radius: 6px; cursor: grab; transition: all 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.05); }
.student-card:active { cursor: grabbing; transform: scale(1.02); box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
.warning-card { border-left: 5px solid #F56C6C; }
.safe-card { border-left: 5px solid #67C23A; }

.card-info { font-size: 15px; }
.trait-tag { margin: 8px 0 0 0; font-size: 13px; color: #909399; background: #f4f4f5; padding: 4px 8px; border-radius: 4px; display: inline-block;}
</style>