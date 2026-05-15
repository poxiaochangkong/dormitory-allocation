//创建路由实例并传递 `routes` 配置
import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', name: 'Login', component: () => import('../views/Login.vue') },
  
  // ====== 管理员端路由 ======
  { path: '/admin/dashboard', name: 'AdminDashboard', component: () => import('../views/AdminDashboard.vue') },
  { path: '/admin/adjust', name: 'AdminAdjust', component: () => import('../views/AdminAdjust.vue') },
  
  // ====== 学生端业务流 ======
  { path: '/student/home', name: 'StudentHome', component: () => import('../views/StudentHome.vue') }, // 新增：学生任务大厅
  { path: '/student/questionnaire', name: 'Questionnaire', component: () => import('../views/Questionnaire.vue') }, // 问卷一：基础与一票否决
  { path: '/student/scene', name: 'ImmersiveScene', component: () => import('../views/ImmersiveScene.vue') }, // 问卷二：沉浸式交互
  { path: '/student/result', name: 'StudentResult', component: () => import('../views/StudentResult.vue') } // 匹配结果
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// 导出路由实例以供 Vue 应用使用
export default router