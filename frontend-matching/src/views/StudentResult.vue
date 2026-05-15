<!-- AdminAdjust.vue -->
<!-- 
  管理员调整后的分配结果展示页面
  - 展示最终分配结果的详细信息
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
      
      <el-descriptions title="分配详情" border :column="2">
        <el-descriptions-item label="宿舍号">南苑 4 栋 302</el-descriptions-item>
        <el-descriptions-item label="宿舍类型">4人间</el-descriptions-item>
        <el-descriptions-item label="室友名单">张三, 李四, 王五</el-descriptions-item>
        <el-descriptions-item label="匹配评级">
          <el-tag type="success" effect="dark">SSS 级完美匹配</el-tag>
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
        description="系统基于您的问卷及一票否决项评估，您与当前室友在作息时间上高度一致，且均排斥深夜游戏噪音。室友中包含性格互补的 E 人与 I 人，安全规避风险极低，能形成良好的寝室氛围。"
        show-icon
        class="mt-20"
      />
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { use } from 'echarts/core'
import { RadarChart } from 'echarts/charts'
import { TitleComponent, TooltipComponent, LegendComponent } from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'
import VChart from 'vue-echarts'

// 注册必须的 ECharts 组件
use([RadarChart, TitleComponent, TooltipComponent, LegendComponent, CanvasRenderer])

// ECharts 雷达图配置项
const chartOption = ref({
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
          value: [92, 85, 98, 90, 95], // 你的超高得分
          name: '您与该宿舍的契合度',
          itemStyle: { color: '#67C23A' },
          areaStyle: { color: 'rgba(103, 194, 58, 0.3)' }
        },
        {
          value: [65, 70, 75, 60, 68], // 平均分作为对比
          name: '全校平均契合度',
          itemStyle: { color: '#909399' },
          areaStyle: { color: 'rgba(144, 147, 153, 0.2)' },
          lineStyle: { type: 'dashed' }
        }
      ]
    }
  ]
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