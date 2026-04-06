//创建路由实例并传递 `routes` 配置
import { createRouter, createWebHistory } from 'vue-router'
// 暂时先只定义登录页，稍后我们建这个文件
const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', name: 'Login', component: () => import('../views/Login.vue') },
  // ====== 学生端路由 [cite: 231] ======
  { path: '/questionnaire', name: 'Questionnaire', component: () => import('../views/Questionnaire.vue') },
  { path: '/student/result', name: 'StudentResult', component: () => import('../views/StudentResult.vue') }, // 匹配结果展示页 [cite: 237]
  // ====== 管理员端路由 [cite: 238] ======
  { path: '/admin/dashboard', name: 'AdminDashboard', component: () => import('../views/AdminDashboard.vue') } // 首页仪表盘 

  
]
// 创建路由实例并传递 `routes` 配置
const router = createRouter({
  history: createWebHistory(),
  routes
})

// 导出路由实例以供 Vue 应用使用
export default router