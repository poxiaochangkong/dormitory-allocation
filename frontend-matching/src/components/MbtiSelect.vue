<!-- MBTI 16型人格选择器 -->
<template>
  <el-select
    v-model="model"
    filterable
    clearable
    :placeholder="placeholder"
    style="width: 100%"
  >
    <el-option
      v-for="m in MBTI_TYPES"
      :key="m.code"
      :label="`${m.code} — ${m.name}`"
      :value="m.code"
    >
      <span style="font-weight:bold; margin-right:8px">{{ m.code }}</span>
      <span style="color:#909399">{{ m.name }}</span>
    </el-option>
  </el-select>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({ modelValue: String, placeholder: { type: String, default: '请选择 MBTI 类型' } })
const emit = defineEmits(['update:modelValue'])
const model = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v)
})

const MBTI_TYPES = [
  { code: 'INTJ', name: '建筑师' },{ code: 'INTP', name: '逻辑学家' },
  { code: 'ENTJ', name: '指挥官' },{ code: 'ENTP', name: '辩论家' },
  { code: 'INFJ', name: '提倡者' },{ code: 'INFP', name: '调停者' },
  { code: 'ENFJ', name: '主人公' },{ code: 'ENFP', name: '竞选者' },
  { code: 'ISTJ', name: '物流师' },{ code: 'ISFJ', name: '守卫者' },
  { code: 'ESTJ', name: '总经理' },{ code: 'ESFJ', name: '执政官' },
  { code: 'ISTP', name: '鉴赏家' },{ code: 'ISFP', name: '探险家' },
  { code: 'ESTP', name: '企业家' },{ code: 'ESFP', name: '表演者' },
]
</script>
