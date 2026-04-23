<template>
  <div class="login-page">
    <el-card class="login-card">
      <div class="login-header">
        <img src="https://element-plus.org/images/element-plus-logo.svg" alt="logo" class="logo">
        <h2>宿舍智能分配系统</h2>
      </div>
      
      <el-tabs v-model="activeRole" stretch>
        <el-tab-pane label="学生登录" name="student"></el-tab-pane>
        <el-tab-pane label="管理端登录" name="admin"></el-tab-pane>
      </el-tabs>

      <el-form label-position="top" class="mt-20">
        <el-form-item :label="activeRole === 'admin' ? '管理员账号' : '学号'">
          <el-input v-model="username" placeholder="请输入内容"></el-input>
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="password" type="password" show-password></el-input>
        </el-form-item>
        
        <el-button type="primary" class="login-btn" @click="handleLogin" :loading="loading">
          {{ activeRole === 'admin' ? '进入管理后台' : '开始匹配问卷' }}
        </el-button>
      </el-form>
      
      <div class="login-footer">
        <p>第一阶段：UI原型已就绪</p>
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'

const router = useRouter()
const activeRole = ref('student')
const username = ref('')
const password = ref('')
const loading = ref(false)

const handleLogin = () => {
  if (!username.value) {
    return ElMessage.warning('请输入账号')
  }
  loading.value = true
  
  // 模拟一个加载效果，显得系统在处理
  setTimeout(() => {
    loading.value = false
    if (activeRole.value === 'admin') {
      ElMessage.success('管理员欢迎回来')
      router.push('/admin/dashboard') // 跳转到管理员大盘
    } else {
      ElMessage.success('登录成功，请开始填写问卷')
      router.push('/student/home') // 【关键修改】：跳转到学生主页
    }
  }, 800)
}
</script>

<style scoped>
.login-page {
  height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); /* 换个更亮的蓝色 */
}
.login-card {
  width: 400px;
  border-radius: 15px;
  box-shadow: 0 8px 20px rgba(0,0,0,0.1);
}
.login-header {
  text-align: center;
  margin-bottom: 20px;
}
.logo { width: 50px; margin-bottom: 10px; }
.mt-20 { margin-top: 20px; }
.login-btn { width: 100%; height: 45px; font-size: 16px; margin-top: 10px; }
.login-footer { text-align: center; color: #909399; font-size: 12px; margin-top: 20px; }
</style>