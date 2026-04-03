<template>
  <div class="login-page">
    <el-card class="login-card">
      <h2>宿舍分配系统</h2>
      <el-form label-position="top">
        <el-form-item label="学号/工号">
          <el-input v-model="user" placeholder="请输入"></el-input>
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="pass" type="password" show-password></el-input>
        </el-form-item>
        <el-button type="primary" style="width: 100%" @click="toHome">登录</el-button>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
const user = ref('')
const pass = ref('')
const router = useRouter()
const toHome = () => {
  // 简单的跳转逻辑
  if(user.value === 'admin') {
    router.push('/admin') // 以后去管理员端
  } else {
    router.push('/student') // 以后去学生端
  }
}
</script>

<style scoped>
.login-page {
  height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
.login-card { width: 350px; border-radius: 12px; }
h2 { text-align: center; margin-bottom: 20px; font-family: "微软雅黑"; }
</style>