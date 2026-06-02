<!-- 信任权重 DJ Mixer — 三滑轨调节器 -->
<template>
  <div class="q-page fade-in">
    <el-card class="q-card">
      <template #header>
        <div class="q-card-header">
          <h3>🎚️ 匹配信任权重调节器</h3>
          <el-tag type="warning" size="small">第 5 步 / 共 5 步</el-tag>
        </div>
      </template>

      <el-steps :active="4" align-center simple class="mb-30">
        <el-step title="传统习惯" /><el-step title="一票否决" />
        <el-step title="性格互补" /><el-step title="沉浸式" /><el-step title="权重调节" />
      </el-steps>

      <div class="mixer-container">
        <!-- 预设按钮 -->
        <div class="presets">
          <el-button-group>
            <el-button :type="preset==='balanced'?'primary':''" @click="applyPreset('balanced')">⚖️ 均衡模式</el-button>
            <el-button :type="preset==='safe'?'primary':''" @click="applyPreset('safe')">🛡️ 安全优先</el-button>
            <el-button :type="preset==='persona'?'primary':''" @click="applyPreset('persona')">🧩 性格优先</el-button>
            <el-button :type="preset==='custom'?'primary':''" @click="applyPreset('custom')">🎛️ 自定义</el-button>
          </el-button-group>
        </div>

        <!-- 三通道滑轨 -->
        <div class="channels">
          <!-- 通道 1: 相似度 -->
          <div class="channel">
            <div class="channel-header">
              <span class="ch-icon">🟢</span>
              <span class="ch-label">生活习惯相似度</span>
              <span class="ch-pct">{{ (weights.sim * 100).toFixed(0) }}%</span>
            </div>
            <p class="ch-desc">作息、卫生、噪音、温度、游戏习惯有多相似</p>
            <div class="fader-track">
              <div class="fader-fill sim-fill" :style="{ width: (weights.sim * 100) + '%' }" />
              <el-slider
                v-model="weights.sim"
                :min="0.1" :max="0.8" :step="0.05"
                :show-tooltip="false"
                @input="onSliderChange('sim')"
                class="fader-slider"
              />
            </div>
          </div>

          <!-- 通道 2: 互补度 -->
          <div class="channel">
            <div class="channel-header">
              <span class="ch-icon">🔵</span>
              <span class="ch-label">性格互补度</span>
              <span class="ch-pct">{{ (weights.comp * 100).toFixed(0) }}%</span>
            </div>
            <p class="ch-desc">MBTI 类型互补程度、社交风格匹配</p>
            <div class="fader-track">
              <div class="fader-fill comp-fill" :style="{ width: (weights.comp * 100) + '%' }" />
              <el-slider
                v-model="weights.comp"
                :min="0.1" :max="0.8" :step="0.05"
                :show-tooltip="false"
                @input="onSliderChange('comp')"
                class="fader-slider"
              />
            </div>
          </div>

          <!-- 通道 3: 否决安全度 -->
          <div class="channel">
            <div class="channel-header">
              <span class="ch-icon">🔴</span>
              <span class="ch-label">一票否决安全度</span>
              <span class="ch-pct">{{ (weights.veto * 100).toFixed(0) }}%</span>
            </div>
            <p class="ch-desc">绝对屏蔽有否决冲突的候选室友</p>
            <div class="fader-track">
              <div class="fader-fill veto-fill" :style="{ width: (weights.veto * 100) + '%' }" />
              <el-slider
                v-model="weights.veto"
                :min="0.1" :max="0.8" :step="0.05"
                :show-tooltip="false"
                @input="onSliderChange('veto')"
                class="fader-slider"
              />
            </div>
          </div>
        </div>

        <!-- 权重分布可视化 -->
        <div class="weight-viz">
          <div class="bar-row">
            <div class="bar sim-bar" :style="{ width: (weights.sim * 100) + '%' }">
              <span v-if="weights.sim >= 0.15">{{ (weights.sim * 100).toFixed(0) }}%</span>
            </div>
            <div class="bar comp-bar" :style="{ width: (weights.comp * 100) + '%' }">
              <span v-if="weights.comp >= 0.15">{{ (weights.comp * 100).toFixed(0) }}%</span>
            </div>
            <div class="bar veto-bar" :style="{ width: (weights.veto * 100) + '%' }">
              <span v-if="weights.veto >= 0.15">{{ (weights.veto * 100).toFixed(0) }}%</span>
            </div>
          </div>
        </div>
      </div>

      <!-- 导航 -->
      <div class="nav-buttons">
        <el-button size="large" @click="$router.push('/student/immersive')">← 上一步</el-button>
        <el-button type="success" size="large" @click="submitWeights" :loading="submitting">
          保存权重并查看匹配结果 🎉
        </el-button>
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitQuestionnaire } from '../api'

const router = useRouter()
const submitting = ref(false)
const preset = ref('balanced')

const weights = reactive({ sim: 0.5, comp: 0.25, veto: 0.25 })

// Normalize: when one slider moves, adjust others proportionally
const onSliderChange = (moved) => {
  const total = weights.sim + weights.comp + weights.veto
  if (Math.abs(total - 1.0) < 0.001) return

  // Distribute remaining proportionally among the two unmoved
  if (moved === 'sim') {
    const remaining = 1.0 - weights.sim
    const ratio = remaining / (weights.comp + weights.veto)
    weights.comp = +(weights.comp * ratio).toFixed(2)
    weights.veto = +(1.0 - weights.sim - weights.comp).toFixed(2)
  } else if (moved === 'comp') {
    const remaining = 1.0 - weights.comp
    const ratio = remaining / (weights.sim + weights.veto)
    weights.sim = +(weights.sim * ratio).toFixed(2)
    weights.veto = +(1.0 - weights.comp - weights.sim).toFixed(2)
  } else {
    const remaining = 1.0 - weights.veto
    const ratio = remaining / (weights.sim + weights.comp)
    weights.sim = +(weights.sim * ratio).toFixed(2)
    weights.comp = +(1.0 - weights.veto - weights.sim).toFixed(2)
  }
  preset.value = 'custom'
}

const applyPreset = (p) => {
  preset.value = p
  if (p === 'balanced') { weights.sim = 0.5; weights.comp = 0.25; weights.veto = 0.25 }
  else if (p === 'safe') { weights.sim = 0.25; weights.comp = 0.15; weights.veto = 0.6 }
  else if (p === 'persona') { weights.sim = 0.3; weights.comp = 0.5; weights.veto = 0.2 }
}

const submitWeights = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }

  submitting.value = true
  try {
    await submitQuestionnaire({
      userId,
      preference: {
        similarityWeight: weights.sim,
        complementarityWeight: weights.comp,
        vetoSafetyWeight: weights.veto
      }
    })
    ElMessage.success('权重已保存！正在跳转到匹配结果...')
    setTimeout(() => router.push('/student/result'), 1500)
  } catch (err) {
    console.error(err)
  } finally { submitting.value = false }
}
</script>

<style scoped>
.q-page { max-width: 800px; margin: 0 auto; }
.q-card { border-radius: 12px; }
.q-card-header { display: flex; justify-content: space-between; align-items: center; }
.q-card-header h3 { margin: 0; }
.mb-30 { margin-bottom: 30px; }

.mixer-container { padding: 10px 0; }

.presets { text-align: center; margin-bottom: 40px; }

.channels { display: flex; flex-direction: column; gap: 36px; margin-bottom: 40px; }

.channel {
  background: #f8f9fb; border-radius: 12px; padding: 20px 24px;
  border: 1px solid #ebeef5;
}
.channel-header { display: flex; align-items: center; gap: 10px; margin-bottom: 6px; }
.ch-icon { font-size: 22px; }
.ch-label { font-size: 16px; font-weight: 700; color: #303133; flex: 1; }
.ch-pct { font-size: 24px; font-weight: 700; color: #409EFF; }
.ch-desc { font-size: 13px; color: #909399; margin-bottom: 12px; padding-left: 32px; }

.fader-track { position: relative; height: 40px; display: flex; align-items: center; padding-left: 32px; }
.fader-fill {
  position: absolute; left: 32px; top: 8px; height: 24px; border-radius: 4px; opacity: 0.35;
}
.sim-fill { background: #67C23A; }
.comp-fill { background: #409EFF; }
.veto-fill { background: #F56C6C; }
.fader-slider { flex: 1; }

.weight-viz { margin: 30px 0; }
.bar-row { display: flex; gap: 4px; height: 48px; border-radius: 8px; overflow: hidden; }
.bar { display: flex; align-items: center; justify-content: center; font-size: 13px; font-weight: 700; color: #fff;
  transition: width 0.3s; min-width: 0; }
.sim-bar { background: #67C23A; }
.comp-bar { background: #409EFF; }
.veto-bar { background: #F56C6C; }

.nav-buttons { display: flex; justify-content: space-between; margin-top: 20px; }
</style>
