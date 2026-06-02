<!-- 沉浸式 3D 宿舍房间 + 环境音效 -->
<template>
  <div class="scene-page fade-in">
    <el-card class="scene-card">
      <template #header>
        <div class="scene-header">
          <h3>🏠 沉浸式生活习惯采集</h3>
          <div class="header-controls">
            <el-button plain size="small" :type="isNight ? 'info' : 'warning'" @click="toggleTimeOfDay">
              {{ isNight ? '🌙 夜间模式' : '☀️ 白天模式' }}
            </el-button>
            <el-tag type="warning" size="small">第 4 步 / 共 5 步</el-tag>
          </div>
        </div>
        <p class="subtitle">
          🖱️ 拖拽鼠标旋转视角 | 🔴 点击闪烁标记回答问题 | 🔊 已采集 {{ answeredCount }} / 9 项
        </p>
      </template>

      <!-- 3D 舞台 -->
      <div
        class="scene-stage"
        ref="stageRef"
        @mousedown="onMouseDown"
        @mousemove="onMouseMove"
        @mouseup="onMouseUp"
        @mouseleave="onMouseUp"
      >
        <div class="scene-3d" :style="{ transform: `rotateX(${rotX}deg) rotateY(${rotY}deg)` }">
          <!-- 地板 -->
          <div class="room-floor"></div>
          <!-- 后墙 -->
          <div class="room-back-wall">
            <div class="window-frame-3d">
              <div class="window-glass-3d" :class="{ 'night-glass': isNight }">
                <div class="window-x-h"></div><div class="window-x-v"></div>
              </div>
            </div>
            <div class="curtain curtain-left"></div>
            <div class="curtain curtain-right"></div>
          </div>
          <!-- 左墙 -->
          <div class="room-left-wall">
            <div class="bed-unit">
              <div class="bed-bunk">
                <div class="bed-mattress"><div class="bed-pillow"></div></div>
              </div>
              <div class="bed-ladder"></div>
            </div>
            <div class="desk-unit">
              <div class="desk-surface"></div>
              <div class="desk-legs"></div>
              <div class="desk-lamp"></div>
            </div>
          </div>
          <!-- 右墙 -->
          <div class="room-right-wall">
            <div class="door-frame-3d">
              <div class="door-panel-3d"><div class="door-knob-3d"></div></div>
            </div>
            <div class="sink-unit">
              <div class="sink-basin"></div>
              <div class="sink-faucet"></div>
            </div>
            <div class="trash-3d">🗑️</div>
            <div class="noise-icon-3d">🔊</div>
          </div>
          <!-- 天花板 -->
          <div class="room-ceiling">
            <div class="ac-vent">❄️</div>
            <div class="ceiling-light" :class="{ 'night-light': isNight }"></div>
          </div>

          <!-- 9 个热点标记 -->
          <div class="hotspot-3d ac-hot" @click.stop="openQuestion('ac')"><el-tooltip content="空调温度" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d bed-hot" @click.stop="openQuestion('bed')"><el-tooltip content="作息动作" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d desk-hot" @click.stop="openQuestion('desk')"><el-tooltip content="桌面整理" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d door-hot" @click.stop="openQuestion('door')"><el-tooltip content="敲门态度" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d curtain-hot" @click.stop="openQuestion('curtain')"><el-tooltip content="窗帘采光" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d trash-hot" @click.stop="openQuestion('trash')"><el-tooltip content="垃圾处理" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d wash-hot" @click.stop="openQuestion('wash')"><el-tooltip content="洗漱时长" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d noise-hot" @click.stop="openQuestion('noise')"><el-tooltip content="噪声耐受" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
          <div class="hotspot-3d balcony-hot" @click.stop="openQuestion('balcony')"><el-tooltip content="阳台潮湿" placement="top"><div class="pulse-dot"></div></el-tooltip></div>
        </div>
      </div>

      <!-- 进度和提交 -->
      <div class="finish-bar">
        <div class="progress-dots">
          <span v-for="i in 9" :key="i" :class="['dot', { filled: progressMap[i-1] }]" />
        </div>
        <span class="done-text">✅ {{ answeredCount }} / 9 项完成</span>
        <el-button type="success" size="large" :disabled="answeredCount < 9" @click="finishScene" :loading="submitting">
          {{ answeredCount >= 9 ? '全部完成 → 进入权重调节' : `还需 ${9 - answeredCount} 项` }}
        </el-button>
      </div>
    </el-card>

    <!-- 问答弹窗 -->
    <el-dialog v-model="dialogVisible" :title="currentQ.title" width="500px" center @open="onDialogOpen">
      <div class="q-content">
        <p class="q-desc">{{ currentQ.desc }}</p>
        <el-slider v-if="currentQ.type === 'slider'"
          v-model="answers[currentQ.key]"
          :min="currentQ.min" :max="currentQ.max"
          show-input show-input-controls :marks="currentQ.marks"
          @input="onSliderInput" />
        <el-radio-group v-if="currentQ.type === 'radio'"
          v-model="answers[currentQ.key]" class="q-radio-list">
          <el-radio v-for="opt in currentQ.options" :key="opt.val" :label="opt.val" border size="large">
            {{ opt.text }}
          </el-radio>
        </el-radio-group>
      </div>
      <template #footer>
        <el-button type="primary" size="large" @click="saveAnswer">确 定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitSceneData } from '../api'
import { useDormAudio } from '../composables/useDormAudio'

const router = useRouter()
const submitting = ref(false)
const dialogVisible = ref(false)
const currentQ = ref({})
const isNight = ref(false)
const { init, toggleNight, setVolume, playClick, dispose } = useDormAudio()

// 3D 旋转
const rotX = ref(-20)
const rotY = ref(-15)
const dragging = ref(false)
const lastX = ref(0)
const lastY = ref(0)
const stageRef = ref(null)

const onMouseDown = (e) => { dragging.value = true; lastX.value = e.clientX; lastY.value = e.clientY }
const onMouseMove = (e) => {
  if (!dragging.value) return
  rotY.value += (e.clientX - lastX.value) * 0.4
  rotX.value -= (e.clientY - lastY.value) * 0.4
  rotX.value = Math.max(-45, Math.min(45, rotX.value))
  lastX.value = e.clientX; lastY.value = e.clientY
}
const onMouseUp = () => { dragging.value = false }

const toggleTimeOfDay = () => {
  isNight.value = !isNight.value
  toggleNight(isNight.value)
}

// 答案 — 初始化为 null 避免计数值 inflated
const answers = reactive({
  s30_acTemp: null, s31_bedAction: null, s32_deskState: null, s33_doorKnock: null,
  s34_curtain: null, s35_trash: null, s36_wash: null, s37_noiseDb: null, s38_balconyWet: null
})

const progressMap = computed(() =>
  Object.values(answers).map(v => v !== null && v !== '')
)
const answeredCount = computed(() => progressMap.value.filter(Boolean).length)

// 噪音滑块实时联动音频
const onSliderInput = (val) => {
  if (currentQ.value.key === 's37_noiseDb' && typeof val === 'number') {
    setVolume((100 - val) / 100)
  }
}

const questionBank = {
  ac: { key: 's30_acTemp', title: '❄️ 空调温度偏好', desc: '夏天你习惯把空调开到多少度？',
    type: 'slider', min: 16, max: 30, marks: { 16:'16°C', 24:'24°C', 30:'30°C' } },
  bed: { key: 's31_bedAction', title: '🛏️ 室友睡觉时的动作', desc: '若室友已入睡，你进入宿舍时会？',
    type: 'radio', options: [{val:'A',text:'轻手轻脚，尽量无声'},{val:'B',text:'正常进出稍加注意'},{val:'C',text:'不刻意改变节奏'}] },
  desk: { key: 's32_deskState', title: '📚 桌面整理习惯', desc: '你的书桌通常是什么状态？',
    type: 'radio', options: [{val:'A',text:'极简整洁，物品归位'},{val:'B',text:'乱中有序，自己能找到'},{val:'C',text:'不拘小节，自由发挥'}] },
  door: { key: 's33_doorKnock', title: '🚪 敲门借物的态度', desc: '室友不敲门直接借用你的东西？',
    type: 'radio', options: [{val:'A',text:'完全无所谓'},{val:'B',text:'可以接受但最好敲门'},{val:'C',text:'非常反感不可接受'}] },
  curtain: { key: 's34_curtain', title: '🪟 窗帘与采光', desc: '白天你希望窗帘保持什么状态？',
    type: 'radio', options: [{val:'A',text:'拉开窗帘阳光通透'},{val:'B',text:'无所谓看大家意见'},{val:'C',text:'拉上保持私密'}] },
  trash: { key: 's35_trash', title: '🗑️ 垃圾处理态度', desc: '垃圾桶满了还没轮到你值日？',
    type: 'radio', options: [{val:'A',text:'主动顺手带下楼扔'},{val:'B',text:'提醒值日同学去倒'},{val:'C',text:'等值日同学自己发现'}] },
  wash: { key: 's36_wash', title: '🚰 洗漱占用时间', desc: '你早晚洗漱占用洗手台的时长？',
    type: 'radio', options: [{val:'A',text:'5分钟内速战速决'},{val:'B',text:'10-15分钟正常'},{val:'C',text:'半小时以上精致护理'}] },
  noise: { key: 's37_noiseDb', title: '🔊 噪声耐受度', desc: '你能忍受的宿舍噪声分贝上限？',
    type: 'slider', min: 30, max: 100, marks: {30:'轻声',60:'交谈',80:'嘈杂',100:'最大'} },
  balcony: { key: 's38_balconyWet', title: '👕 阳台潮湿容忍度', desc: '对洗手间/阳台地面积水的态度？',
    type: 'radio', options: [{val:'A',text:'立刻擦干受不了水渍'},{val:'B',text:'少量积水可以接受'},{val:'C',text:'完全不在意'}] }
}

const openQuestion = (item) => {
  currentQ.value = questionBank[item]
  dialogVisible.value = true
  playClick()
}

const onDialogOpen = () => { playClick() }

const saveAnswer = () => {
  dialogVisible.value = false
  ElMessage.success('已记录')
  playClick()
}

const finishScene = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }
  submitting.value = true
  try {
    // Filter out null values before submitting
    const payload = { userId }
    Object.entries(answers).forEach(([k, v]) => { if (v !== null && v !== '') payload[k] = v })
    await submitSceneData(payload)
    ElMessage.success('沉浸式采集完成！请进入最终权重调节。')
    setTimeout(() => router.push('/student/weights'), 1500)
  } catch (err) { console.error(err) }
  finally { submitting.value = false }
}

onMounted(() => init())
onUnmounted(() => dispose())
</script>

<style scoped>
.scene-page { max-width: 960px; margin: 0 auto; }
.scene-card { border-radius: 12px; }
.scene-header { display: flex; justify-content: space-between; align-items: center; }
.scene-header h3 { margin: 0; }
.header-controls { display: flex; gap: 10px; align-items: center; }
.subtitle { color: #909399; font-size: 13px; margin: 4px 0 0; }

/* 3D 舞台 */
.scene-stage {
  perspective: 900px; width: 100%; height: 480px; cursor: grab;
  background: radial-gradient(ellipse at center, #e8e0d8 0%, #c8b898 100%);
  border-radius: 8px; overflow: hidden; position: relative;
}
.scene-stage:active { cursor: grabbing; }
.scene-3d {
  width: 100%; height: 100%; position: relative;
  transform-style: preserve-3d; transition: transform 0.1s ease-out;
}

/* 🏗️ CSS 3D 房间结构 */
.room-floor {
  position: absolute; bottom: 0; left: 0; right: 0; height: 120px;
  background: linear-gradient(90deg, #c4a574 0%, #d4b88c 50%, #bba070 100%);
  transform: rotateX(75deg); transform-origin: bottom center;
  border-top: 3px solid #8b6914;
}
.room-back-wall {
  position: absolute; top: 0; left: 0; right: 0; bottom: 120px;
  background: linear-gradient(180deg, #f5efe6 0%, #e8dcc8 100%);
  z-index: 0;
}
.room-left-wall {
  position: absolute; top: 0; left: 0; width: 140px; bottom: 120px;
  background: linear-gradient(180deg, #e0d5c5 0%, #d0c0a8 100%);
  transform: rotateY(30deg); transform-origin: right center;
  border-right: 2px solid #bbb; z-index: 1;
}
.room-right-wall {
  position: absolute; top: 0; right: 0; width: 140px; bottom: 120px;
  background: linear-gradient(180deg, #e0d5c5 0%, #d0c0a8 100%);
  transform: rotateY(-30deg); transform-origin: left center;
  border-left: 2px solid #bbb; z-index: 1;
}
.room-ceiling {
  position: absolute; top: 0; left: 0; right: 0; height: 60px;
  background: linear-gradient(180deg, #faf6f0 0%, #e8dcc8 100%);
  transform: rotateX(-60deg); transform-origin: top center; z-index: 2;
}

/* 窗户 */
.window-frame-3d {
  position: absolute; top: 40px; left: 50%; transform: translateX(-50%);
  width: 200px; height: 140px; background: #6b5b4f; border: 6px solid #5a4a3e;
  border-radius: 3px; z-index: 3;
}
.window-glass-3d {
  width: 100%; height: 100%; background: linear-gradient(180deg, #87ceeb 0%, #b8dff0 100%);
  position: relative;
}
.night-glass { background: linear-gradient(180deg, #1a1a4e 0%, #2a2a5e 100%); }
.window-x-h { position: absolute; top: 50%; width: 100%; height: 3px; background: #5a4a3e; }
.window-x-v { position: absolute; left: 50%; height: 100%; width: 3px; background: #5a4a3e; }
.curtain {
  position: absolute; top: 36px; width: 40px; height: 148px;
  background: linear-gradient(180deg, #e8c8a0 0%, #d4a878 50%, #e8c8a0 100%);
  border-radius: 2px; animation: curtain-sway 3s ease-in-out infinite;
}
.curtain-left { left: calc(50% - 135px); transform-origin: top center; animation-delay: 0s; }
.curtain-right { left: calc(50% + 95px); transform-origin: top center; animation-delay: 1.5s; }
@keyframes curtain-sway { 0%,100%{transform:rotate(0deg)} 25%{transform:rotate(2deg)} 75%{transform:rotate(-2deg)} }

/* 床 (左墙) */
.bed-unit { position: absolute; left: 20px; top: 180px; }
.bed-bunk { width: 100px; height: 70px; background: #8B6914; border-radius: 4px; border: 2px solid #6b5010; }
.bed-mattress { margin: 4px; height: 35px; background: linear-gradient(180deg, #f5f5dc, #e8e0c8); border-radius: 2px; position: relative; }
.bed-pillow { position: absolute; right: 8px; top: 4px; width: 25px; height: 18px; background: #fff; border-radius: 50%; }
.bed-ladder { width: 8px; height: 55px; background: #8B6914; margin-left: 92px; margin-top: -55px; border-radius: 1px; }

/* 书桌 (左墙下方) */
.desk-unit { position: absolute; left: 30px; top: 260px; }
.desk-surface { width: 80px; height: 8px; background: #d4a574; border-radius: 2px; }
.desk-legs { width: 6px; height: 40px; background: #8b7355; margin: 0 auto; }
.desk-lamp {
  position: absolute; top: -12px; right: 10px; width: 8px; height: 8px;
  background: #ffd700; border-radius: 50%; box-shadow: 0 0 8px #ffd700;
  animation: lamp-glow 2s ease-in-out infinite;
}
@keyframes lamp-glow { 0%,100%{opacity:0.6} 50%{opacity:1} }

/* 门 (右墙) */
.door-frame-3d { position: absolute; right: 30px; bottom: 50px; width: 60px; height: 150px; background: #6b5b4f; border-radius: 3px 3px 0 0; }
.door-panel-3d { margin: 4px; height: calc(100% - 4px); background: #a0845c; border-radius: 2px 2px 0 0; position: relative; }
.door-knob-3d { position: absolute; top: 65px; right: 6px; width: 7px; height: 7px; background: #ffd700; border-radius: 50%; }

/* 洗手台 */
.sink-unit { position: absolute; right: 100px; bottom: 160px; }
.sink-basin { width: 65px; height: 25px; background: #e8e8e8; border: 2px solid #ccc; border-radius: 0 0 16px 16px; }
.sink-faucet { width: 4px; height: 15px; background: #aaa; margin: -15px auto 0; }

/* 垃圾桶 */
.trash-3d { position: absolute; right: 110px; bottom: 75px; font-size: 24px; }

/* 噪声图标 */
.noise-icon-3d { position: absolute; left: 50%; bottom: 185px; font-size: 18px; }

/* 空调风口 */
.ac-vent {
  position: absolute; top: 15px; right: 120px; font-size: 16px;
  animation: ac-blink 1.5s ease-in-out infinite;
}
@keyframes ac-blink { 0%,100%{opacity:1} 50%{opacity:0.3} }

/* 天花板灯 */
.ceiling-light {
  position: absolute; top: 10px; left: 50%; transform: translateX(-50%);
  width: 50px; height: 16px; background: #fff9e6; border-radius: 8px;
  box-shadow: 0 0 20px rgba(255,249,230,0.8);
}
.night-light { background: #2a2a40; box-shadow: 0 0 5px rgba(100,100,200,0.3); }

/* 热点标记 */
.hotspot-3d { position: absolute; cursor: pointer; z-index: 10; }
.ac-hot { top: 18px; right: 155px; }
.bed-hot { top: 230px; left: 145px; }
.desk-hot { top: 290px; left: 145px; }
.door-hot { top: 240px; right: 110px; }
.curtain-hot { top: 90px; left: 55%; }
.trash-hot { bottom: 290px; right: 160px; }
.wash-hot { bottom: 260px; right: 170px; }
.noise-hot { bottom: 215px; left: 50%; }
.balcony-hot { top: 120px; right: 195px; }

.pulse-dot {
  width: 22px; height: 22px; background: #F56C6C; border-radius: 50%;
  box-shadow: 0 0 0 0 rgba(245,108,108,0.7);
  animation: pulse-ring 1.5s infinite;
}
@keyframes pulse-ring { to { box-shadow: 0 0 0 22px rgba(245,108,108,0); } }

/* 进度条 */
.finish-bar { display: flex; justify-content: space-between; align-items: center; margin-top: 20px; padding: 15px 20px; background: #f0f9eb; border-radius: 8px; gap: 16px; }
.progress-dots { display: flex; gap: 6px; }
.dot { width: 14px; height: 14px; border-radius: 50%; background: #e0e0e0; border: 2px solid #ccc; transition: all 0.3s; }
.dot.filled { background: #67C23A; border-color: #67C23A; }
.done-text { font-size: 15px; font-weight: bold; color: #67C23A; }

/* 弹窗 */
.q-content { padding: 10px 0; }
.q-desc { font-size: 16px; margin-bottom: 24px; font-weight: 600; text-align: center; color: #303133; }
.q-radio-list { display: flex; flex-direction: column; gap: 12px; }
.q-radio-list .el-radio { margin-right: 0; padding: 10px 15px; }
</style>
