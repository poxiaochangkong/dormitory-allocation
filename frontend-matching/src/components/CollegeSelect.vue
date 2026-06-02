<!-- 学院下拉搜索框 -->
<template>
  <el-select
    v-model="model"
    filterable
    clearable
    :placeholder="placeholder"
    style="width: 100%"
  >
    <el-option
      v-for="col in filteredColleges"
      :key="col"
      :label="col"
      :value="col"
    />
  </el-select>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({ modelValue: String, placeholder: { type: String, default: '请选择或搜索学院' } })
const emit = defineEmits(['update:modelValue'])
const model = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v)
})

const COLLEGES = [
  '计算机学院', '软件学院', '电子工程学院', '通信工程学院',
  '数学与统计学院', '物理学院', '化学与化工学院', '生命科学学院',
  '经济与管理学院', '外国语学院', '文学院', '法学院',
  '马克思主义学院', '艺术学院', '体育学院', '医学院',
  '机械工程学院', '土木工程学院', '环境工程学院', '材料科学与工程学院',
]

const filteredColleges = computed(() => COLLEGES)
</script>
