<!-- 性格互补画像 -->
<template>
  <div class="q-page fade-in">
    <el-card class="q-card">
      <template #header>
        <div class="q-card-header">
          <h3>🧩 性格互补画像</h3>
          <el-tag type="info" size="small">第 3 步 / 共 5 步</el-tag>
        </div>
      </template>

      <el-steps :active="2" align-center simple class="mb-30">
        <el-step title="传统习惯" /><el-step title="一票否决" />
        <el-step title="性格互补" /><el-step title="沉浸式" /><el-step title="权重调节" />
      </el-steps>

      <el-alert
        title="💡 为什么要填性格画像？"
        description="系统将在匹配时寻找与您性格互补的室友——例如内向型和外向型搭配，能够互相平衡，形成更和谐的宿舍关系。"
        type="info" show-icon :closable="false" class="mb-20"
      />

      <el-form :model="form" label-width="160px" label-position="top">
        <el-divider content-position="left">🧠 MBTI 人格类型</el-divider>
        <el-form-item label="选择您的 MBTI 类型" required>
          <MbtiSelect v-model="form.personality.p21_mbti" />
          <div class="hint" v-if="form.personality.p21_mbti">
            您选择了 <strong>{{ form.personality.p21_mbti }}</strong> —
            <span class="mbti-trait">{{ mbtiTraits[form.personality.p21_mbti] || '' }}</span>
          </div>
        </el-form-item>

        <el-divider content-position="left">👥 社交偏好</el-divider>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="社交能量水平">
              <el-radio-group v-model="form.personality.p22_socialEnergy">
                <el-radio-button label="A">社交达人（喜热闹、聚会）</el-radio-button>
                <el-radio-button label="B">社交平衡（不排斥也不主动）</el-radio-button>
                <el-radio-button label="C">独行侠（需要安静独处）</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="理想室友关系">
              <el-radio-group v-model="form.personality.p23_idealRelation">
                <el-radio-button label="A">亲密无间的挚友</el-radio-button>
                <el-radio-button label="B">友好适度的交流</el-radio-button>
                <el-radio-button label="C">礼貌保持距离</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
        </el-row>

        <el-divider content-position="left">📋 生活方式</el-divider>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="计划性 (J/P 倾向)">
              <el-radio-group v-model="form.personality.p26_plan">
                <el-radio-button label="A">凡事列清单定计划 (J 型)</el-radio-button>
                <el-radio-button label="B">随遇而安走一步看一步 (P 型)</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="对寝室公约的态度">
              <el-radio-group v-model="form.personality.p28_rules">
                <el-radio-button label="A">必须有严格规章制度</el-radio-button>
                <el-radio-button label="B">差不多就行</el-radio-button>
                <el-radio-button label="C">讨厌被规则束缚</el-radio-button>
              </el-radio-group>
            </el-form-item>
          </el-col>
        </el-row>

        <div class="nav-buttons">
          <el-button size="large" @click="$router.push('/student/veto')">← 上一步</el-button>
          <el-button type="primary" size="large" @click="submitPersonality" :loading="submitting">
            保存并继续 →
          </el-button>
        </div>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { submitQuestionnaire } from '../api'
import MbtiSelect from '../components/MbtiSelect.vue'

const router = useRouter()
const submitting = ref(false)

const mbtiTraits = {
  INTJ:'远见卓识的战略家', INTP:'逻辑严密的思考者', ENTJ:'天生的领导者', ENTP:'聪明好奇的辩论家',
  INFJ:'安静神秘的建议者', INFP:'诗意善良的调停者', ENFJ:'富有魅力的鼓舞者', ENFP:'热情自由的追梦人',
  ISTJ:'务实可靠的管理者', ISFJ:'温柔细腻的守护者', ESTJ:'高效有序的执行者', ESFJ:'热心体贴的照顾者',
  ISTP:'大胆实际的操作者', ISFP:'灵活迷人的艺术家', ESTP:'精力充沛的实干家', ESFP:'自发表演的娱乐家',
}

const form = reactive({
  personality: { p21_mbti: '', p22_socialEnergy: '', p23_idealRelation: '', p26_plan: '', p28_rules: '' }
})

const submitPersonality = async () => {
  const userId = localStorage.getItem('userId')
  if (!userId) { ElMessage.error('请先登录'); router.push('/login'); return }
  if (!form.personality.p21_mbti) { return ElMessage.warning('请选择 MBTI 类型') }

  submitting.value = true
  try {
    await submitQuestionnaire({ userId, personality: form.personality })
    ElMessage.success('性格画像已保存！请继续沉浸式场景采集。')
    router.push('/student/immersive')
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
.hint { margin-top: 8px; font-size: 13px; color: #909399; }
.mbti-trait { color: #409EFF; }
.nav-buttons { display: flex; justify-content: space-between; margin-top: 30px; }
.el-divider { margin: 24px 0 16px; }
</style>
