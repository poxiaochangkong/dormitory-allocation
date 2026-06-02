<!-- 沉浸式场景 — 标准四人寝 + 第一人称漫游 -->
<template>
  <div class="scene-page fade-in">
    <el-card class="scene-card">
      <template #header>
        <div class="scene-header">
          <h3>🏠 沉浸式生活习惯采集</h3>
          <div class="header-controls">
            <el-button plain size="small" :type="isNight ? 'info' : 'warning'" @click="toggleTime">
              {{ isNight ? '🌙 夜间' : '☀️ 白天' }}
            </el-button>
            <el-button plain size="small" @click="resetScene">🔄 重置</el-button>
            <el-tag type="warning" size="small">第 4 / 5 步</el-tag>
          </div>
        </div>
      </template>

      <div class="canvas-wrapper" ref="wrapperRef">
        <canvas ref="canvasRef" class="three-canvas" />
        <!-- 准星 (FPS 模式下显示) -->
        <div class="crosshair" :class="{ visible: pointerLocked }">
          <span class="crosshair-ring"></span>
        </div>
        <!-- 操作提示 -->
        <div class="fps-hint" :class="{ hidden: pointerLocked }">
          🖱️ <span class="hint-key">点击画面</span> 进入第一人称 &nbsp;|&nbsp;
          <span class="hint-key">WASD</span> 移动 &nbsp;|&nbsp;
          <span class="hint-key">E</span> 交互 &nbsp;|&nbsp;
          <span class="hint-key">ESC</span> 退出
        </div>
        <div class="fps-hint locked-hint" :class="{ hidden: !pointerLocked }">
          🎯 <span class="hint-key">WASD</span> 移动 &nbsp;|&nbsp;
          <span class="hint-key">E / 点击</span> 回答 &nbsp;|&nbsp;
          <span class="hint-key">ESC</span> 退出
        </div>
        <!-- 场景快捷传送 -->
        <div class="teleport-btns" :class="{ hidden: pointerLocked }">
          <el-button size="small" round @click="goToScene('center')">🏠 宿舍</el-button>
          <el-button size="small" round @click="goToScene('balcony')">🌿 阳台</el-button>
          <el-button size="small" round type="warning" @click="goToScene('bathroom')">🚿 卫生间</el-button>
        </div>
      </div>

      <div class="finish-bar">
        <div class="progress-dots">
          <span v-for="i in 9" :key="i" :class="['dot', { filled: fillMap[i-1] }]" />
        </div>
        <span class="done-text">✅ {{ answeredCount }} / 9 项</span>
        <el-button type="success" size="large" :disabled="answeredCount < 9" @click="finishScene" :loading="submitting">
          {{ answeredCount >= 9 ? '完成 → 进入权重调节' : `还需 ${9 - answeredCount} 项` }}
        </el-button>
      </div>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="currentQ.title" width="500px" center>
      <div class="q-content">
        <p class="q-desc">{{ currentQ.desc }}</p>
        <el-slider v-if="currentQ.type === 'slider'"
          v-model="answers[currentQ.key]" :min="currentQ.min" :max="currentQ.max"
          show-input show-input-controls :marks="currentQ.marks" />
        <el-radio-group v-if="currentQ.type === 'radio'" v-model="answers[currentQ.key]" class="q-radio-list">
          <el-radio v-for="opt in currentQ.options" :key="opt.val" :label="opt.val" border size="large">{{ opt.text }}</el-radio>
        </el-radio-group>
      </div>
      <template #footer><el-button type="primary" size="large" @click="saveAnswer">确 定</el-button></template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, computed, watch, onMounted, onUnmounted, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitSceneData } from '../api'
import { initScene, toggleNight, disposeScene, setDeskOrganization, exitPointerLock, teleportTo } from '../composables/useDormScene'
import { useDormAudio } from '../composables/useDormAudio'

const router = useRouter()
const submitting = ref(false); const dialogVisible = ref(false); const currentQ = ref({})
const isNight = ref(false); const canvasRef = ref(null); const pointerLocked = ref(false)
const { init: initAudio, toggleNight: audioNight, playClick, playFootstep, playDoor, playFlush, startAmbient, stopAmbient, setVolume, dispose: disposeAudio } = useDormAudio()

const answers = reactive({
  s30_acTemp: null, s31_bedAction: null, s32_deskState: null, s33_privacy: null,
  s34_curtain: null, s35_cleanDuty: null, s36_wash: null, s37_noiseDb: null, s38_balconyWet: null
})
const fillMap = computed(() =>
  ['s30_acTemp','s31_bedAction','s32_deskState','s33_privacy','s34_curtain','s35_cleanDuty','s36_wash','s37_noiseDb','s38_balconyWet']
    .map(k => answers[k] !== null && answers[k] !== '')
)
const answeredCount = computed(() => fillMap.value.filter(Boolean).length)

const questionBank = {
  ac: {
    key: 's30_acTemp', title: '❄️ 空调温度偏好', desc: '夏天你习惯把空调开到多少度？',
    type: 'slider', min: 16, max: 30, marks: { 16:'16°C', 24:'24°C', 30:'30°C' }
  },
  bed: {
    key: 's31_bedAction', title: '🛏️ 室友睡觉时的行为', desc: '有人在睡觉时，你回到宿舍的动作是？',
    type: 'radio', options: [
      {val:'A', text:'轻手轻脚，不发出明显响声'},
      {val:'B', text:'正常走路，但尽量小声说话'},
      {val:'C', text:'不刻意改变，该做什么做什么'}
    ]
  },
  desk: {
    key: 's32_deskState', title: '📚 桌面整理习惯', desc: '你的书桌通常是什么状态？（选择后书桌会变化）',
    type: 'radio', options: [
      {val:'A', text:'书本物品整齐摆放，定期擦拭'},
      {val:'B', text:'东西不少但大致有序'},
      {val:'C', text:'经常杂乱，零食/快递堆满桌面'}
    ]
  },
  privacy: {
    key: 's33_privacy', title: '🔒 个人物品边界感', desc: '室友未经允许拿了你的洗发水/充电器等私人物品？',
    type: 'radio', options: [
      {val:'A', text:'完全无所谓，随便用'},
      {val:'B', text:'可以接受，但希望用完放回原处'},
      {val:'C', text:'非常介意，必须事先征求同意'}
    ]
  },
  curtain: {
    key: 's34_curtain', title: '🪟 窗帘与采光', desc: '白天你希望窗帘保持什么状态？',
    type: 'radio', options: [
      {val:'A', text:'拉开窗帘，让阳光照进来'},
      {val:'B', text:'无所谓，看大家意见'},
      {val:'C', text:'拉上窗帘，保持室内私密'}
    ]
  },
  cleanDuty: {
    key: 's35_cleanDuty', title: '🧹 公共卫生值日态度', desc: '轮到你的值日但你今天特别忙，公共区域垃圾快满了？',
    type: 'radio', options: [
      {val:'A', text:'再忙也会抽空完成值日'},
      {val:'B', text:'先忙完自己的事，晚点再做'},
      {val:'C', text:'今天算了，明天再说吧'}
    ]
  },
  wash: {
    key: 's36_wash', title: '🚿 卫生间使用时间', desc: '你每天早上使用卫生间洗漱/如厕的大概时长？',
    type: 'radio', options: [
      {val:'A', text:'5 分钟内速战速决'},
      {val:'B', text:'10-15 分钟正常节奏'},
      {val:'C', text:'20 分钟以上，习惯慢慢来'}
    ]
  },
  noise: {
    key: 's37_noiseDb', title: '🔊 噪声耐受度', desc: '你能忍受的宿舍噪声分贝上限大约是多少？',
    type: 'slider', min: 30, max: 100, marks: {30:'轻声',60:'交谈',80:'嘈杂',100:'最大'}
  },
  balcony: {
    key: 's38_balconyWet', title: '👕 阳台潮湿容忍度', desc: '看到卫生间或阳台地面有积水/水渍，你的反应是？',
    type: 'radio', options: [
      {val:'A', text:'立刻拖干擦净，不能忍水渍'},
      {val:'B', text:'少量积水可以接受'},
      {val:'C', text:'完全不在意，自然会干'}
    ]
  }
}

let lastDeskState = null
const onHotspot = (key) => {
  currentQ.value = questionBank[key]; dialogVisible.value = true; playClick()
  // 打开对话框时退出指针锁定，让用户可以操作UI
  exitPointerLock()
  // 打开书桌问题时立即应用当前状态
  if (key === 'desk' && lastDeskState) {
    setTimeout(() => setDeskOrganization(lastDeskState === 'C'), 100)
  }
}
const saveAnswer = () => {
  const k = currentQ.value.key
  // 书桌问题：实时反映整洁/杂乱
  if (k === 's32_deskState') {
    lastDeskState = answers[k]
    setDeskOrganization(answers[k] === 'C')
  }
  dialogVisible.value = false; ElMessage.success('已记录'); playClick()
}

const toggleTime = () => { isNight.value = !isNight.value; toggleNight(isNight.value); audioNight(isNight.value) }
const resetScene = () => {
  disposeScene()
  initScene(canvasRef.value, { onHotspot, onMove: (pos) => playFootstep(pos) })
  initAudio(); startAmbient()
  if (isNight.value) toggleNight(true)
  if (lastDeskState) setTimeout(() => setDeskOrganization(lastDeskState === 'C'), 200)
}
const goToScene = (spot) => {
  exitPointerLock()
  teleportTo(spot)
  ElMessage.success({ center: '已传送到宿舍中央', bathroom: '已传送到卫生间 🚿', balcony: '已传送到阳台 🌿', dorm: '已传送到床位区' }[spot] || '已传送')
}

// 噪声耐受度滑块 → 实时调整环境音量
watch(() => answers.s37_noiseDb, (val) => {
  if (val === null || val === undefined) return
  // 30dB → 0.25 音量, 100dB → 1.0 音量
  setVolume(0.2 + (val - 30) / 70 * 0.8)
}, { immediate: false })

const finishScene = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }
  submitting.value = true
  try {
    const payload = { userId }
    Object.entries(answers).forEach(([k, v]) => { if (v !== null && v !== '') payload[k] = v })
    await submitSceneData(payload)
    ElMessage.success('沉浸式采集完成！'); setTimeout(() => router.push('/student/weights'), 1500)
  } catch (err) { console.error(err) } finally { submitting.value = false }
}

onMounted(async () => {
  await nextTick()
  initAudio(); startAmbient()
  initScene(canvasRef.value, { onHotspot, onMove: (pos) => playFootstep(pos) })
  // 追踪指针锁定状态 (用于准星显隐)
  document.addEventListener('pointerlockchange', onPointerLock)
})
onUnmounted(() => {
  disposeScene(); disposeAudio()
  document.removeEventListener('pointerlockchange', onPointerLock)
})
const onPointerLock = () => {
  pointerLocked.value = document.pointerLockElement === canvasRef.value
}
</script>

<style scoped>
.scene-page { max-width: 960px; margin: 0 auto; }
.scene-card { border-radius: 12px; overflow: hidden; }
.scene-header { display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 8px; }
.scene-header h3 { margin: 0; }
.header-controls { display: flex; gap: 8px; align-items: center; }

.canvas-wrapper { width: 100%; height: 550px; border-radius: 8px; overflow: hidden; background: #aac8e0; position: relative; }
.three-canvas { width: 100%; height: 100%; display: block; cursor: crosshair; }

/* 准星 */
.crosshair {
  position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);
  width: 28px; height: 28px; pointer-events: none; opacity: 0; transition: opacity 0.25s;
}
.crosshair.visible { opacity: 0.85; }
.crosshair-ring {
  display: block; width: 100%; height: 100%;
  border: 2px solid rgba(255,255,255,0.85); border-radius: 50%;
  box-shadow: 0 0 4px rgba(0,0,0,0.4), inset 0 0 4px rgba(0,0,0,0.2);
}

.fps-hint {
  position: absolute; bottom: 12px; left: 50%; transform: translateX(-50%);
  background: rgba(0,0,0,0.75); color: #ccc; font-size: 12px; padding: 6px 18px;
  border-radius: 20px; white-space: nowrap; pointer-events: none; transition: opacity 0.4s;
}
.fps-hint.locked-hint { bottom: 36px; background: rgba(0,0,0,0.55); font-size: 11px; }
.fps-hint.hidden { opacity: 0; pointer-events: none; }
.hint-key { color: #67C23A; font-weight: bold; }

.teleport-btns {
  position: absolute; top: 10px; right: 10px; display: flex; gap: 6px;
  z-index: 5; transition: opacity 0.3s;
}
.teleport-btns.hidden { opacity: 0; pointer-events: none; }

.finish-bar { display: flex; justify-content: space-between; align-items: center; margin-top: 12px; padding: 12px 20px; background: #f0f9eb; border-radius: 8px; gap: 16px; }
.progress-dots { display: flex; gap: 6px; }
.dot { width: 14px; height: 14px; border-radius: 50%; background: #e0e0e0; border: 2px solid #ccc; transition: all 0.3s; }
.dot.filled { background: #67C23A; border-color: #67C23A; }
.done-text { font-size: 15px; font-weight: bold; color: #67C23A; }

.q-content { padding: 10px 0; }
.q-desc { font-size: 16px; margin-bottom: 24px; font-weight: 600; text-align: center; color: #303133; }
.q-radio-list { display: flex; flex-direction: column; gap: 12px; }
.q-radio-list .el-radio { margin-right: 0; padding: 10px 15px; }
</style>
