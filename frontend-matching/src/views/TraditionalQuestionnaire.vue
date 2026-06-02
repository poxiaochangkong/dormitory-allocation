<!-- 传统生活习惯问卷 — 基础画像 + 作息 + 习惯 -->
<template>
  <div class="q-page fade-in">
    <el-card class="q-card">
      <template #header>
        <div class="q-card-header">
          <h3>📝 传统生活习惯问卷</h3>
          <el-tag type="info" size="small">第 1 步 / 共 5 步</el-tag>
        </div>
      </template>

      <el-steps :active="0" align-center simple class="mb-30">
        <el-step title="传统习惯" />
        <el-step title="一票否决" />
        <el-step title="性格互补" />
        <el-step title="沉浸式" />
        <el-step title="权重调节" />
      </el-steps>

      <el-form :model="form" label-width="140px" label-position="top">
        <!-- 基础信息 -->
        <el-divider content-position="left">🎓 基础身份信息</el-divider>
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item label="性别" required>
              <el-select v-model="form.basicInfo.gender" placeholder="请选择" style="width:100%">
                <el-option label="男" value="male" />
                <el-option label="女" value="female" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="学院" required>
              <CollegeSelect v-model="form.basicInfo.college" />
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="专业">
              <el-input v-model="form.basicInfo.major" placeholder="如：软件工程" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item label="年级">
              <el-select v-model="form.basicInfo.grade" placeholder="请选择" style="width:100%">
                <el-option label="2026 级" value="2026" />
                <el-option label="2025 级" value="2025" />
                <el-option label="2024 级" value="2024" />
                <el-option label="2023 级" value="2023" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <!-- 作息 -->
        <el-divider content-position="left">⏰ 作息习惯</el-divider>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="入睡时间 (工作日)" required>
              <el-radio-group v-model="form.traditionalHabits.q02_sleepTime">
                <el-radio-button label="A">22:30 前</el-radio-button>
                <el-radio-button label="B">23:30 前</el-radio-button>
                <el-radio-button label="C">00:30 前</el-radio-button>
                <el-radio-button label="D">01:30 后</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="起床时间 (工作日)" required>
              <el-radio-group v-model="form.traditionalHabits.q03_wakeTime">
                <el-radio-button label="A">07:00 前</el-radio-button>
                <el-radio-button label="B">08:00 前</el-radio-button>
                <el-radio-button label="C">09:00 前</el-radio-button>
                <el-radio-button label="D">随课表变动</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
        </el-row>

        <!-- 生活习惯 -->
        <el-divider content-position="left">🧹 生活习惯</el-divider>
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item label="个人卫生习惯">
              <el-select v-model="form.traditionalHabits.q06_hygiene" style="width:100%">
                <el-option label="每天洗澡，比较讲究" value="A" />
                <el-option label="2-3 天一次，正常水平" value="B" />
                <el-option label="偶尔偷懒，不太在意" value="C" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="换洗衣物处理">
              <el-select v-model="form.traditionalHabits.q07_laundry" style="width:100%">
                <el-option label="随换随洗" value="A" />
                <el-option label="堆积周末统一洗" value="B" />
                <el-option label="全部丢洗衣机" value="C" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="宿舍学习频率">
              <el-select v-model="form.traditionalHabits.q08_studyPlace" style="width:100%">
                <el-option label="常在寝室学习" value="A" />
                <el-option label="基本去图书馆/教室" value="B" />
                <el-option label="只在考前突击" value="C" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <!-- 🔥 新增字段 — 算法所需但之前缺失的 -->
        <el-divider content-position="left">🔊 环境偏好 <el-tag type="danger" size="small">影响算法匹配</el-tag></el-divider>
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item label="噪声耐受度 (1=安静 → 5=无所谓)">
              <el-slider v-model="form.traditionalHabits.q04_noiseTolerance" :min="1" :max="5" show-stops :marks="{1:'安静',3:'适中',5:'吵闹'}"/>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="空调温度偏好 (°C)">
              <el-slider v-model="form.traditionalHabits.q05_tempPref" :min="18" :max="30" show-input :marks="{18:'18',24:'24',30:'30'}"/>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="打游戏频率">
              <el-select v-model="form.traditionalHabits.q09_gamingHabit" style="width:100%">
                <el-option label="从不玩" value="A" />
                <el-option label="偶尔玩" value="B" />
                <el-option label="经常玩" value="C" />
                <el-option label="重度玩家" value="D" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <!-- 🔥 新增 — 让否决系统生效的行为特征 -->
        <el-divider content-position="left">🚬 生活特征 <el-tag type="warning" size="small">用于否决项匹配</el-tag></el-divider>
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item label="是否抽烟/饮酒">
              <el-radio-group v-model="form.traditionalHabits.q10_smokeStatus">
                <el-radio-button label="no">不抽烟不喝酒</el-radio-button>
                <el-radio-button label="sometimes">偶尔</el-radio-button>
                <el-radio-button label="often">经常</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="是否养宠物或有此打算">
              <el-radio-group v-model="form.traditionalHabits.q11_petPreference">
                <el-radio-button label="no">不养</el-radio-button>
                <el-radio-button label="want">想养</el-radio-button>
                <el-radio-button label="have">已养</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="带异性朋友来宿舍">
              <el-radio-group v-model="form.traditionalHabits.q12_oppositeSex">
                <el-radio-button label="never">从不</el-radio-button>
                <el-radio-button label="sometimes">偶尔</el-radio-button>
                <el-radio-button label="often">经常</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item style="margin-top:30px">
          <el-button type="primary" size="large" @click="submitForm" :loading="submitting" style="width:100%">
            保存并继续 →
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitQuestionnaire } from '../api'
import CollegeSelect from '../components/CollegeSelect.vue'

const router = useRouter()
const submitting = ref(false)

const form = reactive({
  basicInfo: { gender: '', college: '', major: '', grade: '' },
  traditionalHabits: {
    q02_sleepTime: '', q03_wakeTime: '', q06_hygiene: '', q07_laundry: '', q08_studyPlace: '',
    q04_noiseTolerance: 3, q05_tempPref: 24, q09_gamingHabit: 'B',
    q10_smokeStatus: 'no', q11_petPreference: 'no', q12_oppositeSex: 'never'
  }
})

const submitForm = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }
  if (!form.traditionalHabits.q02_sleepTime) { return ElMessage.warning('请选择入睡时间') }

  submitting.value = true
  try {
    await submitQuestionnaire({
      userId,
      basicInfo: form.basicInfo,
      traditionalHabits: form.traditionalHabits
    })
    ElMessage.success('传统问卷已保存！请继续设置一票否决项。')
    router.push('/student/veto')
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
.mb-30 { margin-bottom: 30px; }
.el-divider { margin: 24px 0 16px; }
</style>
