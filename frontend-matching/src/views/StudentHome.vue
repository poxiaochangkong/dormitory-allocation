<!-- 学生大厅 — 5 步流程导航 -->
<template>
  <div class="home-page fade-in">
    <div class="welcome-banner">
      <h1>👋 欢迎来到新生宿舍智能分配系统</h1>
      <p>完成以下 5 个步骤，AI 将为您匹配最合拍的室友</p>
    </div>

    <!-- 5 步流程 -->
    <el-steps :active="activeStep" align-center finish-status="success" class="flow-steps">
      <el-step title="传统习惯" description="作息与基础信息" />
      <el-step title="一票否决" description="底线红线设置" />
      <el-step title="性格互补" description="MBTI 与社交风格" />
      <el-step title="沉浸式采集" description="3D 场景体验" />
      <el-step title="权重调节" description="DJ Mixer 调参" />
    </el-steps>

    <!-- 流程卡片 -->
    <el-row :gutter="20" class="card-row">
      <el-col :span="8" v-for="card in cards" :key="card.route">
        <el-card shadow="hover" :class="['step-card', { done: card.done }]" @click="$router.push(card.route)">
          <div class="step-badge">{{ card.step }}</div>
          <div class="step-icon">{{ card.icon }}</div>
          <h3>{{ card.title }}</h3>
          <p>{{ card.desc }}</p>
          <el-tag v-if="card.done" type="success" size="small">✓ 已完成</el-tag>
          <el-tag v-else-if="card.step === activeStep + 1" type="warning" size="small">进行中</el-tag>
          <el-tag v-else type="info" size="small">待完成</el-tag>
        </el-card>
      </el-col>
    </el-row>

    <!-- 结果入口 -->
    <div class="result-entry">
      <el-card shadow="hover" class="result-card" @click="$router.push('/student/result')">
        <h2>🎉 查看最终匹配结果</h2>
        <p>完成所有步骤后，查看系统为您智能匹配的神仙室友</p>
      </el-card>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { getQuestionnaireStatus, getMatchResult } from '../api'

const activeStep = ref(0)

const cards = [
  { step: 1, icon: '📝', title: '传统生活习惯', desc: '填写作息、卫生、环境偏好等基础信息', route: '/student/traditional', done: false },
  { step: 2, icon: '🚨', title: '一票否决项', desc: '设置绝对无法容忍的室友行为红线', route: '/student/veto', done: false },
  { step: 3, icon: '🧩', title: '性格互补画像', desc: '选择 MBTI 类型与社交偏好', route: '/student/personality', done: false },
  { step: 4, icon: '🏠', title: '沉浸式场景采集', desc: '3D 宿舍房间体验式问卷', route: '/student/immersive', done: false },
  { step: 5, icon: '🎛️', title: '信任权重调节', desc: 'DJ Mixer 式权重参数调节', route: '/student/weights', done: false },
]

onMounted(async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) return
  try {
    const status = await getQuestionnaireStatus(userId)
    if (status && status.submitted) {
      activeStep.value = Math.max(activeStep.value, 1)
    }
    const result = await getMatchResult(userId)
    if (result && Object.keys(result).length > 0) {
      activeStep.value = 5
    }
  } catch (e) { /* ignore */ }
})
</script>

<style scoped>
.home-page { max-width: 1000px; margin: 0 auto; }

.welcome-banner { text-align: center; margin-bottom: 30px; }
.welcome-banner h1 { font-size: 28px; color: #303133; margin-bottom: 8px; }
.welcome-banner p { font-size: 15px; color: #909399; }

.flow-steps { margin-bottom: 40px; }

.card-row { margin-bottom: 24px; }
.step-card {
  cursor: pointer; text-align: center; border: 2px solid transparent; transition: all 0.3s;
  height: 100%;
}
.step-card:hover { border-color: #409EFF; transform: translateY(-4px); box-shadow: 0 8px 25px rgba(0,0,0,0.1); }
.step-card.done { border-color: #67C23A; background: #f0f9eb; }
.step-badge {
  width: 32px; height: 32px; border-radius: 50%; background: #409EFF; color: #fff;
  display: flex; align-items: center; justify-content: center; margin: 0 auto 10px;
  font-weight: bold; font-size: 14px;
}
.step-card.done .step-badge { background: #67C23A; }
.step-icon { font-size: 36px; margin-bottom: 8px; }
.step-card h3 { font-size: 16px; color: #303133; margin-bottom: 6px; }
.step-card p { font-size: 13px; color: #909399; line-height: 1.5; margin-bottom: 12px; }

.result-entry { text-align: center; }
.result-card { cursor: pointer; padding: 10px 0; }
.result-card:hover { border-color: #67C23A; }
.result-card h2 { margin: 0 0 6px; color: #67C23A; }
.result-card p { color: #909399; margin: 0; }
</style>
