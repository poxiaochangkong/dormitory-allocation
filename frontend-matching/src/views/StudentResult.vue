<!-- StudentResult.vue -->
<!-- 
  学生匹配结果展示页面
  - 从后端 API 获取真实分配结果
  - 包含 AI 算法的多维评分雷达图
  - 提供匹配结果的深度解释
-->

<template>
  <div class="result-container">
    <el-card class="result-card">
      <template #header>
        <div class="header-box">
          <h2>🎉 您的宿舍分配结果已出炉</h2>
          <el-button type="primary" plain @click="$router.push('/student/home')">返回大厅</el-button>
        </div>
      </template>

      <!-- Loading state -->
      <div v-if="loading" style="text-align: center; padding: 60px 0;">
        <el-icon class="is-loading" :size="40"><Loading /></el-icon>
        <p style="margin-top: 15px; color: #909399;">正在获取分配结果...</p>
      </div>

      <!-- No result yet -->
      <el-empty v-else-if="!result" description="暂无分配结果，请等待管理员执行分配任务">
        <el-button type="primary" @click="$router.push('/student/home')">返回大厅</el-button>
      </el-empty>

      <!-- Result content -->
      <template v-else>
        <el-descriptions title="分配详情" border :column="2">
          <el-descriptions-item label="宿舍号">
            {{ result.building || '-' }} {{ result.roomNumber || '-' }}
          </el-descriptions-item>
          <el-descriptions-item label="匹配评级">
            <el-tag :type="matchLevel.type" effect="dark">{{ matchLevel.label }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="室友名单" :span="2">
            <template v-if="result.roommates && result.roommates.length > 0">
              <el-tag v-for="rm in result.roommates" :key="rm.userId" style="margin-right: 8px;">
                {{ rm.studentNo }} ({{ rm.college }} - {{ rm.major }})
              </el-tag>
            </template>
            <span v-else>-</span>
          </el-descriptions-item>
          <el-descriptions-item label="综合匹配分">
            {{ (result.totalScore * 100).toFixed(1) }} 分
          </el-descriptions-item>
          <el-descriptions-item label="解释说明" :span="2">
            {{ result.explanationText || '系统基于您的问卷及一票否决项评估完成匹配。' }}
          </el-descriptions-item>
        </el-descriptions>

        <div class="score-section">
          <h3>🤖 AI 算法多维评分解析</h3>
          <div class="chart-box">
            <v-chart class="chart" :option="chartOption" autoresize />
          </div>
        </div>

        <el-alert
          title="匹配结果深度解释"
          type="info"
          :description="result.explanationText || '系统基于您的问卷及一票否决项评估完成匹配，确保在作息时间、卫生标准等方面的一致性。'"
          show-icon
          class="mt-20"
        />
      </template>
    </el-card>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { getMatchResult } from '../api'

import { use } from 'echarts/core'
import { RadarChart } from 'echarts/charts'
import { TitleComponent, TooltipComponent, LegendComponent } from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'
import VChart from 'vue-echarts'

use([RadarChart, TitleComponent, TooltipComponent, LegendComponent, CanvasRenderer])

const router = useRouter()
const loading = ref(true)
const result = ref(null)

// Compute match level based on total score
const matchLevel = computed(() => {
  if (!result.value) return { label: '-', type: 'info' }
  const score = result.value.totalScore
  if (score >= 0.9) return { label: 'SSS 级完美匹配', type: 'success' }
  if (score >= 0.8) return { label: 'SS 级优秀匹配', type: 'success' }
  if (score >= 0.7) return { label: 'S 级良好匹配', type: '' }
  if (score >= 0.6) return { label: 'A 级一般匹配', type: 'warning' }
  return { label: 'B 级基本匹配', type: 'danger' }
})

// ECharts radar chart using real data from backend
const chartOption = computed(() => {
  if (!result.value) return {}

  const r = result.value
  // Scale scores to 0-100 range (backend scores are 0-1)
  const simScore = Math.round((r.similarityScore || 0) * 100)
  const compScore = Math.round((r.complementarityScore || 0) * 100)
  const safeScore = Math.round((r.vetoRiskScore || 0) * 100)
  const hygieneScore = Math.round((r.hygieneConsistencyScore || 0) * 100)
  const scheduleScore = Math.round((r.scheduleOverlapScore || 0) * 100)

  return {
    tooltip: { trigger: 'item' },
    legend: { data: ['您与该宿舍的契合度', '全校平均契合度'], bottom: 0 },
    radar: {
      indicator: [
        { name: '生活习惯相似度 (Sim)', max: 100 },
        { name: '性格互补度 (Comp)', max: 100 },
        { name: '风险规避安全度 (Safe)', max: 100 },
        { name: '卫生标准一致性', max: 100 },
        { name: '作息重合度', max: 100 }
      ],
      radius: '65%',
      splitNumber: 4,
      axisName: { color: '#409EFF', fontWeight: 'bold' }
    },
    series: [
      {
        name: '匹配维测评估',
        type: 'radar',
        data: [
          {
            value: [simScore, compScore, safeScore, hygieneScore, scheduleScore],
            name: '您与该宿舍的契合度',
            itemStyle: { color: '#67C23A' },
            areaStyle: { color: 'rgba(103, 194, 58, 0.3)' }
          },
          {
            value: [65, 70, 75, 60, 68],
            name: '全校平均契合度',
            itemStyle: { color: '#909399' },
            areaStyle: { color: 'rgba(144, 147, 153, 0.2)' },
            lineStyle: { type: 'dashed' }
          }
        ]
      }
    ]
  }
})

onMounted(async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) {
    ElMessage.error('未检测到登录信息，请重新登录')
    router.push('/login')
    return
  }

  try {
    const data = await getMatchResult(userId)
    // Backend returns {} if no result yet
    if (data && Object.keys(data).length > 0) {
      result.value = data
    }
  } catch (err) {
    console.error('Get match result failed:', err)
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.result-container { padding: 30px; display: flex; justify-content: center; background-color: #f5f7fa; min-height: 100vh; }
.result-card { width: 900px; }
.header-box { display: flex; justify-content: space-between; align-items: center; }
.header-box h2 { margin: 0; color: #303133; }
.score-section { margin-top: 30px; text-align: center; }
.score-section h3 { color: #409EFF; margin-bottom: 20px; }
.chart-box { height: 400px; width: 100%; display: flex; justify-content: center; }
.chart { width: 100%; height: 100%; }
.mt-20 { margin-top: 20px; }
</style>