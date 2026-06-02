<!-- 主布局：侧边栏 + 顶部 + 内容区 -->
<template>
  <el-container class="app-layout">
    <!-- 侧边栏 -->
    <el-aside :width="isCollapse ? '64px' : '220px'" class="app-aside">
      <div class="logo-box" @click="$router.push(role === 'admin' ? '/admin/dashboard' : '/student/home')">
        <span class="logo-icon">🏠</span>
        <span v-show="!isCollapse" class="logo-text">宿舍分配系统</span>
      </div>

      <el-menu
        :default-active="currentRoute"
        :collapse="isCollapse"
        :collapse-transition="false"
        background-color="#304156"
        text-color="#bfcbd9"
        active-text-color="#409EFF"
        router
      >
        <template v-if="role === 'student'">
          <el-menu-item index="/student/home">
            <el-icon><HomeFilled /></el-icon>
            <span>大厅首页</span>
          </el-menu-item>
          <el-menu-item index="/student/traditional">
            <el-icon><Document /></el-icon>
            <span>传统生活习惯</span>
          </el-menu-item>
          <el-menu-item index="/student/veto">
            <el-icon><WarningFilled /></el-icon>
            <span>一票否决项</span>
          </el-menu-item>
          <el-menu-item index="/student/personality">
            <el-icon><UserFilled /></el-icon>
            <span>性格互补画像</span>
          </el-menu-item>
          <el-menu-item index="/student/immersive">
            <el-icon><VideoCameraFilled /></el-icon>
            <span>沉浸式场景采集</span>
          </el-menu-item>
          <el-menu-item index="/student/weights">
            <el-icon><SetUp /></el-icon>
            <span>信任权重调节</span>
          </el-menu-item>
          <el-menu-item index="/student/result">
            <el-icon><TrophyBase /></el-icon>
            <span>匹配结果</span>
          </el-menu-item>
        </template>
        <template v-else>
          <el-menu-item index="/admin/dashboard">
            <el-icon><DataBoard /></el-icon>
            <span>仪表盘</span>
          </el-menu-item>
          <el-menu-item index="/admin/adjust">
            <el-icon><Switch /></el-icon>
            <span>手动调整</span>
          </el-menu-item>
          <el-menu-item index="/admin/rules">
            <el-icon><Setting /></el-icon>
            <span>规则配置</span>
          </el-menu-item>
        </template>
      </el-menu>
    </el-aside>

    <!-- 主体 -->
    <el-container>
      <!-- 顶部栏 -->
      <el-header class="app-header">
        <div class="header-left">
          <el-button text @click="isCollapse = !isCollapse">
            <el-icon :size="20"><Fold v-if="!isCollapse" /><Expand v-else /></el-icon>
          </el-button>
          <span class="page-title">{{ pageTitle }}</span>
        </div>
        <div class="header-right">
          <span class="user-info">
            <el-icon><User /></el-icon>
            {{ role === 'admin' ? '管理员' : '学生' }} | {{ userId?.slice(0,10) }}...
          </span>
          <el-button type="danger" plain size="small" @click="handleLogout">退出登录</el-button>
        </div>
      </el-header>

      <!-- 内容区 -->
      <el-main class="app-main">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import {
  HomeFilled, Document, WarningFilled, UserFilled, VideoCameraFilled,
  SetUp, TrophyBase, DataBoard, Switch, Setting, Fold, Expand, User
} from '@element-plus/icons-vue'

const router = useRouter()
const route = useRoute()

const isCollapse = ref(false)
const role = computed(() => localStorage.getItem('role') || 'student')
const userId = computed(() => localStorage.getItem('userId') || '')
const currentRoute = computed(() => route.path)

const pageTitles = {
  '/student/home': '🏠 大厅首页',
  '/student/traditional': '📝 传统生活习惯问卷',
  '/student/veto': '🚨 一票否决项配置',
  '/student/personality': '🧩 性格互补画像',
  '/student/immersive': '🏠 沉浸式生活场景采集',
  '/student/weights': '🎚️ 匹配权重调节',
  '/student/result': '🎉 宿舍分配结果',
  '/admin/dashboard': '📊 管理员仪表盘',
  '/admin/adjust': '🔄 手动调整分配',
  '/admin/rules': '⚙️ 分配规则配置',
}
const pageTitle = computed(() => pageTitles[route.path] || '宿舍智能分配系统')

const handleLogout = () => {
  localStorage.removeItem('token')
  localStorage.removeItem('userId')
  localStorage.removeItem('role')
  router.push('/login')
}
</script>

<style scoped>
.app-layout { height: 100vh; }

.app-aside {
  background: #304156;
  overflow-x: hidden;
  transition: width 0.3s;
}
.logo-box {
  height: 56px; display: flex; align-items: center; justify-content: center;
  gap: 8px; cursor: pointer; color: #fff; font-weight: bold; font-size: 16px;
  border-bottom: 1px solid rgba(255,255,255,0.1);
}
.logo-icon { font-size: 24px; }
.logo-text { white-space: nowrap; }

.app-aside :deep(.el-menu) { border-right: none; }

.app-header {
  background: #fff; display: flex; align-items: center; justify-content: space-between;
  border-bottom: 1px solid #e6e6e6; height: 56px; padding: 0 20px;
  box-shadow: 0 1px 4px rgba(0,0,0,0.05);
}
.header-left { display: flex; align-items: center; gap: 12px; }
.page-title { font-size: 17px; font-weight: 600; color: #303133; }
.header-right { display: flex; align-items: center; gap: 16px; }
.user-info { font-size: 13px; color: #909399; display: flex; align-items: center; gap: 4px; }

.app-main {
  background: #f0f2f5; overflow-y: auto; padding: 20px;
}
</style>
