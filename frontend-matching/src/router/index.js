//创建路由实例并传递 `routes` 配置
import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', name: 'Login', component: () => import('../views/Login.vue'), meta: { guest: true } },

  // ====== 管理员端路由 ======
  { path: '/admin/dashboard', name: 'AdminDashboard', component: () => import('../views/AdminDashboard.vue'), meta: { requiresAuth: true, role: 'admin' } },
  { path: '/admin/adjust', name: 'AdminAdjust', component: () => import('../views/AdminAdjust.vue'), meta: { requiresAuth: true, role: 'admin' } },

  // ====== 学生端业务流 ======
  { path: '/student/home', name: 'StudentHome', component: () => import('../views/StudentHome.vue'), meta: { requiresAuth: true } },
  { path: '/student/questionnaire', name: 'Questionnaire', component: () => import('../views/Questionnaire.vue'), meta: { requiresAuth: true } },
  { path: '/student/scene', name: 'ImmersiveScene', component: () => import('../views/ImmersiveScene.vue'), meta: { requiresAuth: true } },
  { path: '/student/result', name: 'StudentResult', component: () => import('../views/StudentResult.vue'), meta: { requiresAuth: true } }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// Navigation guard: check authentication
router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const role = localStorage.getItem('role')

  // Public routes (login page)
  if (to.meta.guest) {
    // Already logged in? Redirect to appropriate home
    if (token) {
      if (role === 'admin') {
        return next('/admin/dashboard')
      }
      return next('/student/home')
    }
    return next()
  }

  // Protected routes require token
  if (to.meta.requiresAuth && !token) {
    return next('/login')
  }

  // Admin-only routes
  if (to.meta.role === 'admin' && role !== 'admin') {
    return next('/login')
  }

  next()
})

// 导出路由实例以供 Vue 应用使用
export default router