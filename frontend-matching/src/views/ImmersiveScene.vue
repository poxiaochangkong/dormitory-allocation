<!-- 沉浸式场景问卷组件 -->
<template>
  <div class="scene-container">
    <el-card class="scene-card">
      <template #header>
        <div class="header-box">
          <h2>🏠 沉浸式生活习惯采集</h2>
          <div>
            <el-button type="primary" plain @click="$router.push('/student/questionnaire')">切换至传统列表模式</el-button>
            <el-button plain @click="$router.push('/student/home')">返回大厅</el-button>
            <el-button type="danger" plain @click="handleLogout">退出登录</el-button>
          </div>
        </div>
        <p class="subtitle">请点击房间内的 9 个闪烁标记，完成最终场景画像</p>
      </template>

      <div class="room-wrapper">
        <img src="https://images.unsplash.com/photo-1555854877-bab0e564b8d5?q=80&w=800&h=500&auto=format&fit=crop" alt="宿舍背景" class="room-bg" />

        <div class="hotspot ac-hotspot" @click="openQuestion('ac')"><el-tooltip content="空调偏好" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot bed-hotspot" @click="openQuestion('bed')"><el-tooltip content="作息动作" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot desk-hotspot" @click="openQuestion('desk')"><el-tooltip content="桌面状态" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot door-hotspot" @click="openQuestion('door')"><el-tooltip content="敲门态度" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot curtain-hotspot" @click="openQuestion('curtain')"><el-tooltip content="窗帘采光" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot trash-hotspot" @click="openQuestion('trash')"><el-tooltip content="垃圾处理" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot wash-hotspot" @click="openQuestion('wash')"><el-tooltip content="洗漱时长" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot noise-hotspot" @click="openQuestion('noise')"><el-tooltip content="噪声耐受" placement="top"><div class="pulse-point"></div></el-tooltip></div>
        <div class="hotspot balcony-hotspot" @click="openQuestion('balcony')"><el-tooltip content="阳台潮湿" placement="top"><div class="pulse-point"></div></el-tooltip></div>
      </div>

      <div style="text-align: center; margin-top: 30px;">
        <el-button type="success" size="large" @click="finishScene">全部采集完成，开始AI匹配</el-button>
      </div>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="currentQ.title" width="450px" center>
      <div class="question-content">
        <p class="q-desc">{{ currentQ.desc }}</p>
        
        <el-slider v-if="currentQ.type === 'slider'" v-model="answers[currentQ.key]" :min="currentQ.min" :max="currentQ.max" show-input></el-slider>
        
        <el-radio-group v-if="currentQ.type === 'radio'" v-model="answers[currentQ.key]" class="q-radio-group">
          <el-radio v-for="opt in currentQ.options" :key="opt.val" :label="opt.val" border>{{ opt.text }}</el-radio>
        </el-radio-group>
      </div>
      <template #footer>
        <span class="dialog-footer">
          <el-button type="primary" @click="saveAnswer">确 定</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitSceneData } from '../api'

const router = useRouter()
const submitting = ref(false)

// Logout: clear session and return to login page
const handleLogout = () => {
  localStorage.removeItem('token')
  localStorage.removeItem('userId')
  localStorage.removeItem('role')
  router.push('/login')
}

// 存储沉浸式场景答案
const answers = reactive({
  s30_acTemp: 24, s31_bedAction: '', s32_deskState: '', s33_doorKnock: '', 
  s34_curtain: '', s35_trash: '', s36_wash: '', s37_noiseDb: 60, s38_balconyWet: ''
})

const dialogVisible = ref(false)
const currentQ = ref({})

// 完整的9个问题库
const questionBank = {
  ac: { key: 's30_acTemp', title: '❄️ 空调偏好', desc: '夏天你最习惯的温度是多少度？', type: 'slider', min: 16, max: 30 },
  bed: { key: 's31_bedAction', title: '🛏️ 作息动作', desc: '若室友在睡觉，你进入宿舍的动作是？', type: 'radio', options: [{ val: 'A', text: '轻手轻脚，尽量无声' }, { val: 'B', text: '正常进出，稍微注意' }, { val: 'C', text: '不刻意改变节奏' }] },
  desk: { key: 's32_deskState', title: '📚 桌面状态', desc: '你的桌面通常呈现哪种状态？', type: 'radio', options: [{ val: 'A', text: '极简空旷' }, { val: 'B', text: '乱中有序' }, { val: 'C', text: '自由发挥' }] },
  door: { key: 's33_doorKnock', title: '🚪 敲门态度', desc: '对"未敲门直接借东西"的室友态度是？', type: 'radio', options: [{ val: 'A', text: '完全无所谓' }, { val: 'B', text: '可以接受，但最好敲门' }, { val: 'C', text: '非常反感' }] },
  curtain: { key: 's34_curtain', title: '🪟 窗帘采光', desc: '白天你希望宿舍保持什么状态？', type: 'radio', options: [{ val: 'A', text: '拉开窗帘保持通透' }, { val: 'B', text: '无所谓' }, { val: 'C', text: '喜欢常年拉窗帘' }] },
  trash: { key: 's35_trash', title: '🗑️ 垃圾处理', desc: '垃圾满了但没轮到你值日，你会？', type: 'radio', options: [{ val: 'A', text: '随手带走' }, { val: 'B', text: '提醒值日生' }, { val: 'C', text: '视而不见' }] },
  wash: { key: 's36_wash', title: '🚰 洗漱时长', desc: '你早晚洗漱占用台面的时间大约是？', type: 'radio', options: [{ val: 'A', text: '5分钟内' }, { val: 'B', text: '10-15分钟' }, { val: 'C', text: '半小时以上' }] },
  noise: { key: 's37_noiseDb', title: '🔊 噪声耐受', desc: '环境噪声达到多少分贝时会无法忍受？', type: 'slider', min: 30, max: 100 },
  balcony: { key: 's38_balconyWet', title: '👕 阳台潮湿', desc: '对洗手间/阳台地面有积水的忍受力？', type: 'radio', options: [{ val: 'A', text: '立刻拖干，受不了水渍' }, { val: 'B', text: '正常积水能接受' }, { val: 'C', text: '毫不在意' }] }
}

const openQuestion = (item) => { currentQ.value = questionBank[item]; dialogVisible.value = true }
const saveAnswer = () => { dialogVisible.value = false; ElMessage.success('记录成功！') }

const finishScene = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) {
    ElMessage.error('未检测到登录信息，请重新登录')
    router.push('/login')
    return
  }

  submitting.value = true
  try {
    // Submit scene data to backend
    await submitSceneData({
      userId: userId,
      ...answers
    })

    ElMessage.success('正在为您执行全局匹配算法...')
    setTimeout(() => { router.push('/student/result') }, 2000)
  } catch (err) {
    console.error('Submit scene data failed:', err)
  } finally {
    submitting.value = false
  }
}
</script>

<style scoped>
.scene-container { padding: 30px; display: flex; justify-content: center; }
.scene-card { width: 900px; }
.header-box { display: flex; justify-content: space-between; align-items: center; }
.subtitle { color: #909399; font-size: 14px; margin-top: 5px; }

.room-wrapper {
  position: relative; width: 800px; height: 500px; margin: 20px auto;
  border-radius: 12px; overflow: hidden; box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}
.room-bg { width: 100%; height: 100%; object-fit: cover; }

/* 9个热区的散布排版 */
.hotspot { position: absolute; cursor: pointer; }
.ac-hotspot { top: 10%; right: 20%; }
.bed-hotspot { top: 40%; left: 15%; }
.desk-hotspot { bottom: 30%; left: 35%; }
.door-hotspot { top: 30%; right: 5%; }
.curtain-hotspot { top: 15%; left: 50%; }
.trash-hotspot { bottom: 15%; right: 30%; }
.wash-hotspot { bottom: 10%; left: 10%; }
.noise-hotspot { top: 50%; right: 40%; }
.balcony-hotspot { top: 10%; right: 40%; }

.pulse-point {
  width: 20px; height: 20px; background-color: #F56C6C; border-radius: 50%;
  box-shadow: 0 0 0 0 rgba(245, 108, 108, 0.7);
  animation: pulse 1.5s infinite cubic-bezier(0.66, 0, 0, 1);
}
@keyframes pulse { to { box-shadow: 0 0 0 20px rgba(245, 108, 108, 0); } }

.q-desc { font-size: 16px; margin-bottom: 20px; font-weight: bold; text-align: center;}
.q-radio-group { display: flex; flex-direction: column; gap: 15px; }
</style>