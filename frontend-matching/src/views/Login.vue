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

      <!-- Login form -->
      <el-form v-if="!showRegister" label-position="top" class="mt-20">
        <el-form-item :label="activeRole === 'admin' ? '管理员账号' : '学号'">
          <el-input v-model="username" placeholder="请输入内容"></el-input>
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="password" type="password" show-password></el-input>
        </el-form-item>
        
        <el-button type="primary" class="login-btn" @click="handleLogin" :loading="loading">
          {{ activeRole === 'admin' ? '进入管理后台' : '开始匹配问卷' }}
        </el-button>

        <div v-if="activeRole === 'student'" class="register-link">
          <el-button type="text" @click="showRegister = true">还没有账号？点击注册</el-button>
        </div>
      </el-form>

      <!-- Register form (student only) -->
      <el-form v-else label-position="top" class="mt-20">
        <el-form-item label="学号">
          <el-input v-model="regForm.studentNo" placeholder="请输入学号"></el-input>
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="regForm.password" type="password" show-password placeholder="请输入密码"></el-input>
        </el-form-item>
        <el-form-item label="确认密码">
          <el-input v-model="regForm.confirmPassword" type="password" show-password placeholder="请再次输入密码"></el-input>
        </el-form-item>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="性别">
              <el-select v-model="regForm.gender" placeholder="请选择" style="width: 100%">
                <el-option label="男" value="male" />
                <el-option label="女" value="female" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="年级">
              <el-input v-model="regForm.grade" placeholder="如：2026" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="学院">
              <el-input v-model="regForm.college" placeholder="如：计算机学院" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="专业">
              <el-input v-model="regForm.major" placeholder="如：软件工程" />
            </el-form-item>
          </el-col>
        </el-row>

        <el-button type="primary" class="login-btn" @click="handleRegister" :loading="loading">
          注册账号
        </el-button>
        <div class="register-link">
          <el-button type="text" @click="showRegister = false">已有账号？返回登录</el-button>
        </div>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { studentLogin, adminLogin, studentRegister } from '../api'

const router = useRouter()
const activeRole = ref('student')
const username = ref('')
const password = ref('')
const loading = ref(false)
const showRegister = ref(false)

const regForm = reactive({
  studentNo: '',
  password: '',
  confirmPassword: '',
  gender: '',
  college: '',
  major: '',
  grade: ''
})

const handleLogin = async () => {
  if (!username.value) {
    return ElMessage.warning('请输入账号')
  }
  if (!password.value) {
    return ElMessage.warning('请输入密码')
  }

  loading.value = true
  try {
    let res
    if (activeRole.value === 'admin') {
      res = await adminLogin(username.value, password.value)
    } else {
      res = await studentLogin(username.value, password.value)
    }

    // Store auth info to localStorage
    localStorage.setItem('token', res.token)
    localStorage.setItem('userId', res.userId)
    localStorage.setItem('role', res.role)

    if (activeRole.value === 'admin') {
      ElMessage.success('管理员欢迎回来')
      router.push('/admin/dashboard')
    } else {
      ElMessage.success('登录成功')
      router.push('/student/home')
    }
  } catch (err) {
    // Error already handled by axios interceptor (ElMessage.error)
    console.error('Login failed:', err)
  } finally {
    loading.value = false
  }
}

const handleRegister = async () => {
  if (!regForm.studentNo) {
    return ElMessage.warning('请输入学号')
  }
  if (!regForm.password) {
    return ElMessage.warning('请输入密码')
  }
  if (regForm.password !== regForm.confirmPassword) {
    return ElMessage.warning('两次密码输入不一致')
  }

  loading.value = true
  try {
    const { studentNo, password, gender, college, major, grade } = regForm
    const res = await studentRegister({ studentNo, password, gender, college, major, grade })

    ElMessage.success('注册成功！请登录')
    // Auto-fill login form with registered student number
    username.value = regForm.studentNo
    password.value = ''
    showRegister.value = false
  } catch (err) {
    console.error('Register failed:', err)
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-page {
  height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
}
.login-card {
  width: 420px;
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
.register-link { text-align: center; margin-top: 12px; }
</style>