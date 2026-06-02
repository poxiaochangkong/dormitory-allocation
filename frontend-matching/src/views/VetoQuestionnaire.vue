<!-- 一票否决项独立页面 — 卡片式布局 -->
<template>
  <div class="q-page fade-in">
    <el-card class="q-card">
      <template #header>
        <div class="q-card-header">
          <h3>🚨 一票否决项</h3>
          <el-tag type="danger" size="small">第 2 步 / 共 5 步</el-tag>
        </div>
      </template>

      <el-alert
        title="⚠️ 重要说明"
        description="开启下方任意一项，意味着：如果潜在室友有此行为，系统将绝对不会将你们分配在同一宿舍。请谨慎选择。"
        type="error" show-icon :closable="false" class="mb-20"
      />

      <el-steps :active="1" align-center simple class="mb-30">
        <el-step title="传统习惯" /><el-step title="一票否决" />
        <el-step title="性格互补" /><el-step title="沉浸式" /><el-step title="权重调节" />
      </el-steps>

      <el-row :gutter="20">
        <el-col :span="12" v-for="item in vetoItems" :key="item.key">
          <el-card
            shadow="hover"
            :class="['veto-card', { 'veto-active': form[item.key] }]"
            @click="form[item.key] = !form[item.key]"
          >
            <div class="veto-card-content">
              <div class="veto-left">
                <span class="veto-icon">{{ item.icon }}</span>
              </div>
              <div class="veto-right">
                <div class="veto-name">{{ item.name }}</div>
                <div class="veto-desc">{{ item.desc }}</div>
              </div>
              <div class="veto-toggle">
                <el-switch
                  v-model="form[item.key]"
                  active-color="#F56C6C"
                  :active-text="form[item.key] ? '已开启' : ''"
                  @click.stop
                />
              </div>
            </div>
          </el-card>
        </el-col>
      </el-row>

      <div class="nav-buttons">
        <el-button size="large" @click="$router.push('/student/traditional')">← 上一步</el-button>
        <el-button type="primary" size="large" @click="submitVeto" :loading="submitting">
          保存并继续 →
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

const vetoItems = [
  { key: 'v11_smokeAlcohol', icon: '🚬', name: '室内吸烟 / 酗酒', desc: '室友在宿舍内吸烟或经常饮酒，影响室内空气环境和他人健康。' },
  { key: 'v12_midnightGaming', icon: '🎮', name: '深夜开麦打游戏', desc: '室友在深夜开着麦克风大声打游戏，严重影响他人休息。' },
  { key: 'v13_loudSpeaker', icon: '📢', name: '长期外放音频', desc: '室友长时间外放音乐、视频或语音通话，制造持续噪音。' },
  { key: 'v14_oppositeSex', icon: '👫', name: '频繁带异性入宿', desc: '室友频繁带异性朋友进入宿舍，影响他人隐私和生活。' },
  { key: 'v15_badHygiene', icon: '🦨', name: '严重卫生问题', desc: '室友个人卫生极差，不洗澡、不洗衣，产生异味或招引虫鼠。' },
  { key: 'v16_overDemand', icon: '🛒', name: '过度索取', desc: '室友习惯性要求代取快递、带饭等，且不懂感恩，形成单方面索取。' },
  { key: 'v17_boundary', icon: '🔍', name: '社交边界侵犯', desc: '室友未经允许翻动他人物品、擅自使用私人物品等越界行为。' },
  { key: 'v19_pets', icon: '🐱', name: '宿舍内养宠物', desc: '室友在宿舍饲养猫、狗、仓鼠等宠物，可能产生异味、毛发或噪音问题。' },
]

const form = reactive({
  v11_smokeAlcohol: false, v12_midnightGaming: false, v13_loudSpeaker: false,
  v14_oppositeSex: false, v15_badHygiene: false, v16_overDemand: false,
  v17_boundary: false, v19_pets: false
})

const submitVeto = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }

  submitting.value = true
  try {
    await submitQuestionnaire({ userId, vetoSettings: form })
    ElMessage.success('否决项已保存！请继续性格画像。')
    router.push('/student/personality')
  } catch (err) {
    console.error(err)
  } finally { submitting.value = false }
}
</script>

<style scoped>
.q-page { max-width: 900px; margin: 0 auto; }
.q-card { border-radius: 12px; }
.q-card-header { display: flex; justify-content: space-between; align-items: center; }
.q-card-header h3 { margin: 0; }
.mb-20 { margin-bottom: 20px; }
.mb-30 { margin-bottom: 30px; }

.veto-card { cursor: pointer; transition: all 0.3s; border: 2px solid transparent; margin-bottom: 12px; }
.veto-card:hover { border-color: #409EFF; }
.veto-active { border-color: #F56C6C !important; background: #fef0f0; }
.veto-card-content { display: flex; align-items: center; gap: 16px; }
.veto-icon { font-size: 32px; }
.veto-name { font-size: 15px; font-weight: 700; color: #303133; }
.veto-desc { font-size: 13px; color: #909399; margin-top: 4px; }
.veto-toggle { margin-left: auto; }

.nav-buttons { display: flex; justify-content: space-between; margin-top: 30px; }
</style>
