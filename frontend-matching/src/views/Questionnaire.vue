<!-- 
  前端——一票否决项问卷页面，使用了 Vue 3 和 Element Plus UI 库。 
  学生可以在此页面填写他们对室友行为的容忍度（包括一票否决项）以及一些基本的生活习惯偏好。
  提交后，数据将被发送到后端进行处理，以便更好地匹配室友。-->

<template>
  <div class="questionnaire-container">
    <el-card class="box-card">
      <template #header>
        <div class="card-header">
          <h2>📝 宿舍生活习惯与一票否决设置</h2>
          <span class="subtitle">请真实填写，我们将为您匹配最合适的室友（系统严格保密）</span>
        </div>
      </template>

      <el-form :model="form" label-width="120px" label-position="top">
        
        <h3 class="section-title">🚨 一票否决项设置 (绝不能容忍的室友行为)</h3>
        <el-collapse v-model="activeNames">
          <el-collapse-item title="🎮 噪音与作息维度的红线" name="1">
            <el-form-item label="能否容忍：深夜频繁敲击机械键盘/开麦打游戏？">
              <el-radio-group v-model="form.veto.noiseGaming">
                <el-radio label="accept">可以接受</el-radio>
                <el-radio label="conditional">偶尔可以</el-radio>
                <el-radio label="veto" style="color: red; font-weight: bold;">一票否决</el-radio>
              </el-radio-group>
            </el-form-item>
            <el-form-item label="能否容忍：在寝室长时间大声外放音频/视频通话？">
              <el-radio-group v-model="form.veto.speaker">
                <el-radio label="accept">可以接受</el-radio>
                <el-radio label="veto" style="color: red; font-weight: bold;">一票否决</el-radio>
              </el-radio-group>
            </el-form-item>
          </el-collapse-item>

          <el-collapse-item title="🚬 卫生与不良嗜好红线" name="2">
             <el-form-item label="能否容忍：室内吸烟 / 经常酗酒？">
              <el-switch v-model="form.veto.smoke" active-text="一票否决" inactive-text="可以接受" active-color="#ff4949"></el-switch>
            </el-form-item>
            <el-form-item label="能否容忍：个人卫生极差（如长期不洗澡/衣服堆积发臭）？">
              <el-switch v-model="form.veto.hygiene" active-text="一票否决" inactive-text="可以接受" active-color="#ff4949"></el-switch>
            </el-form-item>
          </el-collapse-item>

          <el-collapse-item title="🙅 社交边界与性格红线" name="3">
            <el-form-item label="能否容忍：缺乏边界感（乱翻私人物品 / 强行要求帮忙带饭拿快递）？">
              <el-radio-group v-model="form.veto.boundary">
                <el-radio label="accept">无所谓</el-radio>
                <el-radio label="veto" style="color: red; font-weight: bold;">一票否决</el-radio>
              </el-radio-group>
            </el-form-item>
             <el-form-item label="能否容忍：极度自我中心（不愿承担寝室公共卫生、漠视室友休息）？">
              <el-switch v-model="form.veto.selfish" active-text="一票否决" inactive-text="可以接受" active-color="#ff4949"></el-switch>
            </el-form-item>
          </el-collapse-item>
        </el-collapse>

        <el-divider></el-divider>

        <h3 class="section-title">🔍 自我生活习惯画像</h3>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="您通常的入睡时间？">
              <el-time-select v-model="form.sleepTime" start="21:00" step="00:30" end="03:00" placeholder="选择时间" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="您的空调温度偏好？">
              <el-slider v-model="form.temperature" :min="16" :max="30" show-input></el-slider>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item>
          <el-button type="primary" size="large" @click="submitForm" style="width: 100%; margin-top: 20px;">提交问卷与匹配偏好</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router' 
const router = useRouter()


const activeNames = ref(['1', '2', '3']) // 默认展开所有面板展示工作量

const form = reactive({
  veto: {
    noiseGaming: '',
    speaker: '',
    smoke: false,
    hygiene: false,
    boundary: '',
    selfish: false
  },
  sleepTime: '',
  temperature: 24
})

const submitForm = () => {
  console.log('提交的数据：', form)
  // 弹出提示
  ElMessage.success('问卷提交成功！正在通过AI算法为您匹配室友...')
  
  // 延迟2秒跳转，模拟算法计算过程（演示效果极佳！）
  setTimeout(() => {
    router.push('/student/result')
  }, 2000)
}
</script>

<style scoped>
.questionnaire-container {
  padding: 30px;
  background-color: #f5f7fa;
  min-height: 100vh;
}
.box-card {
  max-width: 800px;
  margin: 0 auto;
  border-radius: 10px;
}
.card-header h2 {
  margin: 0;
  color: #303133;
}
.subtitle {
  font-size: 14px;
  color: #909399;
  margin-top: 5px;
  display: block;
}
.section-title {
  color: #409EFF;
  border-bottom: 2px solid #ecf5ff;
  padding-bottom: 10px;
  margin-top: 30px;
  margin-bottom: 20px;
}
</style>