<!-- 结果展示 — Hero 横幅 + 室友卡片 + 雷达图 -->
<template>
  <div class="result-page fade-in">
    <!-- Loading -->
    <div v-if="loading" class="loading-box">
      <el-icon class="is-loading" :size="40"><Loading /></el-icon>
      <p>正在获取分配结果...</p>
    </div>

    <!-- No Result -->
    <el-empty v-else-if="!result" description="暂无分配结果">
      <p style="color:#909399;font-size:13px;margin-bottom:16px;">请先在大厅完成全部 5 步问卷，然后由管理员执行分配任务</p>
      <el-button type="primary" @click="$router.push('/student/home')">返回大厅</el-button>
    </el-empty>

    <template v-else>
      <!-- Hero 横幅 -->
      <div :class="['hero-banner', 'hero-' + matchLevel.key]">
        <div class="hero-badge">{{ matchLevel.badge }}</div>
        <h1>{{ result.building }} · {{ result.roomNumber }}</h1>
        <p class="hero-sub">{{ matchLevel.label }} — 综合匹配度 {{ (result.totalScore * 100).toFixed(1) }}%</p>
        <div class="hero-score-ring">
          <svg viewBox="0 0 100 100">
            <circle cx="50" cy="50" r="42" fill="none" stroke="rgba(255,255,255,0.25)" stroke-width="6"/>
            <circle cx="50" cy="50" r="42" fill="none" stroke="#fff" stroke-width="6"
              :stroke-dasharray="264" :stroke-dashoffset="264 * (1 - result.totalScore)"
              stroke-linecap="round" transform="rotate(-90 50 50)"/>
            <text x="50" y="55" text-anchor="middle" fill="#fff" font-size="20" font-weight="bold">
              {{ (result.totalScore * 100).toFixed(0) }}%
            </text>
          </svg>
        </div>
      </div>

      <!-- 室友卡片 -->
      <div class="roommates-section">
        <h2>👥 您的室友</h2>
        <el-row :gutter="20">
          <el-col :span="8" v-for="(rm, i) in (result.roommates || [])" :key="rm.userId">
            <el-card shadow="hover" class="roommate-card">
              <div class="rm-avatar">{{ rm.studentNo?.slice(-2) || '?' }}</div>
              <h4>{{ rm.studentNo }}</h4>
              <p class="rm-info">{{ rm.college }}</p>
              <p class="rm-info">{{ rm.major }}</p>
              <el-tag size="small" :type="['','success','warning'][i%3]">室友 {{ i+1 }}</el-tag>
            </el-card>
          </el-col>
          <el-col :span="8" v-if="!result.roommates || result.roommates.length === 0">
            <el-card shadow="hover" class="roommate-card empty-roommate">
              <div class="rm-avatar" style="background:#dcdfe6">?</div>
              <h4>等待室友</h4>
              <p class="rm-info">系统暂未分配室友</p>
            </el-card>
          </el-col>
        </el-row>
      </div>

      <!-- 雷达图 -->
      <el-card class="chart-card">
        <template #header><h3>🤖 AI 多维评分解析</h3></template>
        <div class="chart-box">
          <v-chart class="chart" :option="chartOption" autoresize />
        </div>
        <div class="chart-legend">
          <span class="dot-green"></span> 您的匹配度
          <span class="dot-gray"></span> 全校平均值
        </div>
      </el-card>

      <!-- 匹配解释 -->
      <el-card class="explain-card">
        <template #header><h3>📋 匹配深度解读</h3></template>
        <div class="explain-list">
          <div class="explain-item" v-if="result.similarityScore > 0.5">
            ✅ 生活习惯相似度高（{{ (result.similarityScore * 100).toFixed(0) }}%），作息和卫生习惯接近
          </div>
          <div class="explain-item" v-else>
            ⚠️ 生活习惯有一定差异，但已在可接受范围
          </div>
          <div class="explain-item" v-if="result.complementarityScore > 0.5">
            ✅ MBTI 性格互补度高（{{ (result.complementarityScore * 100).toFixed(0) }}%），性格互相补充
          </div>
          <div class="explain-item" v-else>
            ℹ️ 性格特征接近，更注重生活习惯匹配
          </div>
          <div class="explain-item" v-if="result.vetoRiskScore > 0.8">
            ✅ 否决安全检查通过（{{ (result.vetoRiskScore * 100).toFixed(0) }}%），无红线冲突
          </div>
          <div class="explain-item" v-else>
            ⚠️ 存在轻微否决风险，但已优先回避主要红线
          </div>
        </div>
        <p class="explain-full">{{ result.explanationText || '系统基于您的问卷数据及一票否决项完成了智能匹配。' }}</p>
      </el-card>
    </template>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { getMatchResult } from '../api'
import { use } from 'echarts/core'
import { RadarChart } from 'echarts/charts'
import { TitleComponent, TooltipComponent, LegendComponent } from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'
import VChart from 'vue-echarts'

use([RadarChart, TitleComponent, TooltipComponent, LegendComponent, CanvasRenderer])

const loading = ref(true)
const result = ref(null)

const matchLevel = computed(() => {
  if (!result.value) return { key: 'none', label: '-', badge: '-' }
  const s = result.value.totalScore || 0
  if (s >= 0.85) return { key: 'sss', label: 'SSS 完美匹配', badge: '🏆' }
  if (s >= 0.75) return { key: 'ss', label: 'SS 优秀匹配', badge: '🌟' }
  if (s >= 0.65) return { key: 's', label: 'S 良好匹配', badge: '✨' }
  if (s >= 0.55) return { key: 'a', label: 'A 基础匹配', badge: '👍' }
  return { key: 'b', label: 'B 基本匹配', badge: '📌' }
})

const chartOption = computed(() => {
  if (!result.value) return {}
  const r = result.value
  const sim = Math.round((r.similarityScore || 0) * 100)
  const comp = Math.round((r.complementarityScore || 0) * 100)
  const veto = Math.round((r.vetoRiskScore || 0) * 100)
  const hygiene = Math.round((r.hygieneConsistencyScore || 0) * 100)
  const schedule = Math.round((r.scheduleOverlapScore || 0) * 100)

  const avgSim = Math.round((r.avgSimilarityScore || 0.6) * 100)
  const avgComp = Math.round((r.avgComplementarityScore || 0.5) * 100)
  const avgVeto = Math.round((r.avgVetoRiskScore || 0.7) * 100)
  const avgHyg = Math.round((r.avgSimilarityScore || 0.6) * 100)
  const avgSch = Math.round((r.avgComplementarityScore || 0.5) * 100)

  return {
    tooltip: { trigger: 'item' },
    legend: { data: ['您的得分', '全校平均'], bottom: 0 },
    radar: {
      indicator: [
        { name: '生活习惯\n相似度', max: 100 },
        { name: '性格互补度', max: 100 },
        { name: '否决安全度', max: 100 },
        { name: '卫生一致性', max: 100 },
        { name: '作息重合度', max: 100 },
      ],
      radius: '65%',
      splitNumber: 4,
      axisName: { color: '#409EFF', fontWeight: 'bold' }
    },
    series: [{
      name: '匹配分析', type: 'radar',
      data: [
        { value: [sim, comp, veto, hygiene, schedule], name: '您的得分',
          itemStyle: { color: '#67C23A' }, areaStyle: { color: 'rgba(103,194,58,0.3)' } },
        { value: [avgSim, avgComp, avgVeto, avgHyg, avgSch], name: '全校平均',
          itemStyle: { color: '#909399' }, areaStyle: { color: 'rgba(144,147,153,0.15)' },
          lineStyle: { type: 'dashed' } },
      ]
    }]
  }
})

onMounted(async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); return }
  try {
    const data = await getMatchResult(userId)
    if (data && Object.keys(data).length > 0) result.value = data
  } catch (e) { console.error(e) }
  finally { loading.value = false }
})
</script>

<style scoped>
.result-page { max-width: 900px; margin: 0 auto; }

.loading-box { text-align: center; padding: 80px 0; color: #909399; }
.loading-box p { margin-top: 16px; }

/* Hero */
.hero-banner {
  text-align: center; padding: 50px 30px; border-radius: 16px; color: #fff; position: relative; overflow: hidden;
  margin-bottom: 30px;
}
.hero-sss { background: linear-gradient(135deg, #f5af19, #f12711); }
.hero-ss  { background: linear-gradient(135deg, #667eea, #764ba2); }
.hero-s   { background: linear-gradient(135deg, #409EFF, #36d1dc); }
.hero-a   { background: linear-gradient(135deg, #67C23A, #85ce61); }
.hero-b   { background: linear-gradient(135deg, #909399, #b4b4b4); }
.hero-badge { font-size: 48px; }
.hero-banner h1 { font-size: 32px; margin: 10px 0 6px; }
.hero-sub { font-size: 16px; opacity: 0.9; margin: 0; }
.hero-score-ring { width: 100px; height: 100px; margin: 20px auto 0; }

/* Roommate cards */
.roommates-section { margin-bottom: 30px; }
.roommates-section h2 { margin-bottom: 16px; color: #303133; }
.roommate-card { text-align: center; }
.rm-avatar {
  width: 56px; height: 56px; border-radius: 50%; background: linear-gradient(135deg, #409EFF, #67C23A);
  color: #fff; display: flex; align-items: center; justify-content: center; font-size: 20px;
  font-weight: bold; margin: 0 auto 12px;
}
.roommate-card h4 { margin: 0 0 4px; }
.rm-info { font-size: 13px; color: #909399; margin: 2px 0; }

/* Chart */
.chart-card { margin-bottom: 30px; }
.chart-box { height: 380px; display: flex; justify-content: center; }
.chart { width: 100%; height: 100%; }
.chart-legend { text-align: center; font-size: 13px; color: #909399; margin-top: 8px; }
.dot-green { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #67C23A; margin-right: 4px; }
.dot-gray { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #909399; margin-left: 16px; margin-right: 4px; }

/* Explanation */
.explain-card { margin-bottom: 30px; }
.explain-list { margin-bottom: 16px; }
.explain-item { padding: 8px 0; font-size: 15px; border-bottom: 1px dashed #ebeef5; }
.explain-item:last-child { border-bottom: none; }
.explain-full { color: #606266; line-height: 1.8; margin: 0; }
</style>
