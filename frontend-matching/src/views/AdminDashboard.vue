<template>
  <div class="admin-container">
    <h2>📊 管理员控制台 - 宿舍分配系统</h2>
    <el-row :gutter="20">
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="已导入学生总数" :value="1254" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="空闲宿舍床位" :value="1300" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="已完成问卷人数" :value="982" />
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <el-statistic title="当前分配任务数" :value="3" />
        </el-card>
      </el-col>
    </el-row>

    <el-card class="mt-20">
      <template #header>
        <div class="card-header">
          <span>🚀 最新分配任务列表</span>
          <el-button type="primary">创建新分配任务</el-button>
        </div>
      </template>
      <el-table :data="tableData" style="width: 100%">
        <el-table-column prop="taskId" label="任务编号" width="180" />
        <el-table-column prop="college" label="目标学院" width="180" />
        <el-table-column prop="status" label="当前状态">
          <template #default="scope">
            <el-tag :type="scope.row.type">{{ scope.row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="progress" label="算法执行进度">
          <template #default="scope">
            <el-progress :percentage="scope.row.progress" />
          </template>
        </el-table-column>
        <el-table-column label="操作">
          <el-button size="small" type="success" plain>查看结果</el-button>
          <el-button size="small" type="warning" plain>人工调整</el-button>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const tableData = ref([
  { taskId: 'TASK-2026-001', college: '计算机学院', status: '全局分配完成', type: 'success', progress: 100 },
  { taskId: 'TASK-2026-002', college: '外国语学院', status: '算法评分中...', type: 'warning', progress: 65 },
  { taskId: 'TASK-2026-003', college: '机械工程学院', status: '等待数据导入', type: 'info', progress: 0 },
])
</script>

<style scoped>
.admin-container { padding: 30px; background-color: #f0f2f5; min-height: 100vh; }
.mt-20 { margin-top: 20px; }
.card-header { display: flex; justify-content: space-between; align-items: center; }
</style>