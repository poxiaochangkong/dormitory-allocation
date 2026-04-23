<!-- 
  前端——一票否决项问卷页面，使用了 Vue 3 和 Element Plus UI 库。 
  学生可以在此页面填写他们对室友行为的容忍度（包括一票否决项）以及一些基本的生活习惯偏好。
  提交后，数据将被发送到后端进行处理，以便更好地匹配室友。-->
<template>
  <div class="questionnaire-container">
    <el-card class="box-card">
      <template #header>
        <div class="card-header">
          <h2>📝 基础画像与底线红线设置</h2>
          <el-button type="primary" plain @click="$router.push('/student/home')">返回大厅</el-button>
        </div>
      </template>

      <el-form :model="form" label-width="140px" label-position="top">
        
        <el-collapse v-model="activeNames">
          <el-collapse-item title="🎓 第一部分：基础身份信息" name="1">
            <el-row :gutter="20">
              <el-col :span="8">
                <el-form-item label="性别">
                  <el-select v-model="form.basicInfo.gender" placeholder="请选择">
                    <el-option label="男" value="male" />
                    <el-option label="女" value="female" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="学院">
                  <el-input v-model="form.basicInfo.college" placeholder="如：计算机学院" />
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="专业">
                  <el-input v-model="form.basicInfo.major" placeholder="如：软件工程" />
                </el-form-item>
              </el-col>
            </el-row>
          </el-collapse-item>

          <el-collapse-item title="⏰ 第二部分：传统生活习惯" name="2">
            <el-row :gutter="20">
              <el-col :span="12">
                <el-form-item label="入睡时间 (工作日)">
                  <el-radio-group v-model="form.traditionalHabits.q02_sleepTime">
                    <el-radio label="A">22:30前</el-radio>
                    <el-radio label="B">23:30前</el-radio>
                    <el-radio label="C">0:30前</el-radio>
                    <el-radio label="D">1:30后</el-radio>
                  </el-radio-group>
                </el-form-item>
              </el-col>
              <el-col :span="12">
                <el-form-item label="起床时间 (工作日)">
                  <el-radio-group v-model="form.traditionalHabits.q03_wakeTime">
                    <el-radio label="A">7:00前</el-radio>
                    <el-radio label="B">8:00前</el-radio>
                    <el-radio label="C">9:00前</el-radio>
                    <el-radio label="D">随课表变动</el-radio>
                  </el-radio-group>
                </el-form-item>
              </el-col>
            </el-row>
            <el-row :gutter="20">
              <el-col :span="8">
                <el-form-item label="个人卫生习惯">
                  <el-select v-model="form.traditionalHabits.q06_hygiene">
                    <el-option label="每天洗澡" value="A" />
                    <el-option label="2-3天一次" value="B" />
                    <el-option label="偶尔偷懒" value="C" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="换洗衣物处理">
                  <el-select v-model="form.traditionalHabits.q07_laundry">
                    <el-option label="随换随洗" value="A" />
                    <el-option label="堆积到周末统一洗" value="B" />
                    <el-option label="全部丢洗衣机" value="C" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="宿舍学习频率">
                  <el-select v-model="form.traditionalHabits.q08_studyPlace">
                    <el-option label="喜欢在寝室学习" value="A" />
                    <el-option label="基本去图书馆" value="B" />
                    <el-option label="只在考前突击" value="C" />
                  </el-select>
                </el-form-item>
              </el-col>
            </el-row>
          </el-collapse-item>

          <el-collapse-item title="🚨 第三部分：一票否决项 (绝不能容忍的室友行为)" name="3">
            <el-alert title="开启以下选项代表：若对方有此行为，系统将绝对不会把你们分配在同一宿舍。" type="error" show-icon style="margin-bottom: 20px;" />
            <el-row :gutter="20">
              <el-col :span="12"><el-form-item label="室内吸烟/酗酒"><el-switch v-model="form.vetoSettings.v11_smokeAlcohol" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="深夜开麦打游戏"><el-switch v-model="form.vetoSettings.v12_midnightGaming" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="长期外放音频"><el-switch v-model="form.vetoSettings.v13_loudSpeaker" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="带异性入宿"><el-switch v-model="form.vetoSettings.v14_oppositeSex" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="严重卫生隐患"><el-switch v-model="form.vetoSettings.v15_badHygiene" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="过度索取(带饭/拿快递)"><el-switch v-model="form.vetoSettings.v16_overDemand" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="社交边界侵犯(乱翻东西)"><el-switch v-model="form.vetoSettings.v17_boundary" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
              <el-col :span="12"><el-form-item label="寝室私养宠物"><el-switch v-model="form.vetoSettings.v19_pets" active-text="一票否决" active-color="#ff4949" /></el-form-item></el-col>
            </el-row>
          </el-collapse-item>

          <el-collapse-item title="🧩 第四部分：性格与互补画像" name="4">
            <el-row :gutter="20">
              <el-col :span="8">
                <el-form-item label="MBTI人格类型">
                  <el-input v-model="form.personality.p21_mbti" placeholder="如：INTJ" />
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="社交能量">
                  <el-select v-model="form.personality.p22_socialEnergy">
                    <el-option label="社交达人(喜热闹)" value="A" />
                    <el-option label="社交平衡" value="B" />
                    <el-option label="独行侠(需安静)" value="C" />
                  </el-select>
                </el-form-item>
              </el-col>
              <el-col :span="8">
                <el-form-item label="理想室友关系">
                  <el-select v-model="form.personality.p23_idealRelation">
                    <el-option label="亲密无间挚友" value="A" />
                    <el-option label="友好适度交流" value="B" />
                    <el-option label="礼貌保持距离" value="C" />
                  </el-select>
                </el-form-item>
              </el-col>
            </el-row>
            <el-row :gutter="20">
              <el-col :span="12">
                <el-form-item label="计划性">
                  <el-radio-group v-model="form.personality.p26_plan">
                    <el-radio label="A">凡事列清单(J型)</el-radio>
                    <el-radio label="B">随遇而安(P型)</el-radio>
                  </el-radio-group>
                </el-form-item>
              </el-col>
              <el-col :span="12">
                <el-form-item label="寝室公约态度">
                  <el-radio-group v-model="form.personality.p28_rules">
                    <el-radio label="A">必须有严格规章</el-radio>
                    <el-radio label="B">差不多就行</el-radio>
                    <el-radio label="C">讨厌束缚</el-radio>
                  </el-radio-group>
                </el-form-item>
              </el-col>
            </el-row>
          </el-collapse-item>
        </el-collapse>

        <el-form-item>
          <el-button type="primary" size="large" @click="submitForm" style="width: 100%; margin-top: 30px;">
            保存配置并返回大厅
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

const router = useRouter()
const activeNames = ref(['1', '2', '3', '4']) // 默认展开所有

const form = reactive({
  basicInfo: { gender: '', college: '', major: '' },
  traditionalHabits: { q02_sleepTime: '', q03_wakeTime: '', q06_hygiene: '', q07_laundry: '', q08_studyPlace: '' },
  vetoSettings: { v11_smokeAlcohol: false, v12_midnightGaming: false, v13_loudSpeaker: false, v14_oppositeSex: false, v15_badHygiene: false, v16_overDemand: false, v17_boundary: false, v19_pets: false },
  personality: { p21_mbti: '', p22_socialEnergy: '', p23_idealRelation: '', p26_plan: '', p28_rules: '' }
})

const submitForm = () => {
  console.log('提交的基础问卷数据：', form)
  ElMessage.success('基础画像已保存！请继续完成第二阶段。')
  setTimeout(() => {
    router.push('/student/home') // 保存后退回大厅
  }, 1000)
}
</script>

<style scoped>
.questionnaire-container { padding: 30px; background-color: #f5f7fa; min-height: 100vh; }
.box-card { max-width: 1000px; margin: 0 auto; border-radius: 10px; }
.card-header { display: flex; justify-content: space-between; align-items: center; }
.card-header h2 { margin: 0; color: #303133; }
</style>