<!-- 管理员分配规则配置 -->
<template>
  <div class="q-page fade-in">
    <el-card class="q-card">
      <template #header><h3>⚙️ 分配规则配置</h3></template>

      <el-alert title="提示" description="这些规则权重将作为全局默认值应用到所有新建的分配任务中。" type="info" show-icon :closable="false" class="mb-20"/>

      <el-form label-width="160px">
        <el-divider content-position="left">📊 匹配算法默认权重</el-divider>
        <el-form-item label="相似度权重 (Sim)">
          <el-slider v-model="rule.similarityWeight" :min="0.1" :max="0.8" :step="0.05" show-input />
          <span class="hint">生活习惯的相似程度在匹配中的比重</span>
        </el-form-item>
        <el-form-item label="互补度权重 (Comp)">
          <el-slider v-model="rule.complementarityWeight" :min="0.1" :max="0.8" :step="0.05" show-input />
          <span class="hint">MBTI 性格互补程度在匹配中的比重</span>
        </el-form-item>
        <el-form-item label="否决安全权重 (Veto)">
          <el-slider v-model="rule.vetoSafetyWeight" :min="0.1" :max="0.8" :step="0.05" show-input />
          <span class="hint">一票否决项的严格程度在匹配中的比重</span>
        </el-form-item>

        <el-divider content-position="left">📋 分配默认参数</el-divider>
        <el-form-item label="每间宿舍人数">
          <el-input-number v-model="rule.defaultCapacity" :min="2" :max="8" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" size="large" @click="saveRule" :loading="saving">💾 保存规则</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { saveAllocationRule } from '../api'

const saving = ref(false)
const rule = reactive({
  similarityWeight: 0.50,
  complementarityWeight: 0.25,
  vetoSafetyWeight: 0.25,
  defaultCapacity: 4
})

const saveRule = async () => {
  saving.value = true
  try {
    await saveAllocationRule({
      similarityWeight: rule.similarityWeight,
      complementarityWeight: rule.complementarityWeight,
      vetoSafetyWeight: rule.vetoSafetyWeight,
      defaultCapacity: rule.defaultCapacity
    })
    ElMessage.success('规则已保存')
  } catch (err) {
    console.error(err)
  } finally { saving.value = false }
}
</script>

<style scoped>
.q-page { max-width: 800px; margin: 0 auto; }
.q-card { border-radius: 12px; }
.mb-20 { margin-bottom: 20px; }
.hint { font-size: 12px; color: #909399; display: block; margin-top: 4px; }
.el-divider { margin: 24px 0 16px; }
</style>
