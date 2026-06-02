// 路由配置：嵌套路由 + 侧边栏布局
import { createRouter, createWebHistory } from 'vue-router'
import MainLayout from '../layouts/MainLayout.vue'

const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', name: 'Login', component: () => import('../views/Login.vue'), meta: { guest: true } },

  // ====== 学生端 (嵌套路由) ======
  {
    path: '/student',
    component: MainLayout,
    meta: { requiresAuth: true },
    children: [
      { path: '', redirect: '/student/home' },
      { path: 'home', name: 'StudentHome', component: () => import('../views/StudentHome.vue') },
      { path: 'traditional', name: 'Traditional', component: () => import('../views/TraditionalQuestionnaire.vue') },
      { path: 'veto', name: 'VetoQuestionnaire', component: () => import('../views/VetoQuestionnaire.vue') },
      { path: 'personality', name: 'Personality', component: () => import('../views/PersonalityQuestionnaire.vue') },
      { path: 'immersive', name: 'ImmersiveScene', component: () => import('../views/ImmersiveScene.vue') },
      { path: 'weights', name: 'WeightMixer', component: () => import('../views/WeightMixer.vue') },
      { path: 'result', name: 'StudentResult', component: () => import('../views/StudentResult.vue') },
    ]
  },

  // ====== 管理员端 (嵌套路由) ======
  {
    path: '/admin',
    component: MainLayout,
    meta: { requiresAuth: true, role: 'admin' },
    children: [
      { path: '', redirect: '/admin/dashboard' },
      { path: 'dashboard', name: 'AdminDashboard', component: () => import('../views/AdminDashboard.vue') },
      { path: 'adjust', name: 'AdminAdjust', component: () => import('../views/AdminAdjust.vue') },
      { path: 'rules', name: 'RuleConfig', component: () => import('../views/RuleConfig.vue') },
    ]
  },
]

const router = createRouter({ history: createWebHistory(), routes })

router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const role = localStorage.getItem('role')

  if (to.meta.guest) {
    if (token) {
      return next(role === 'admin' ? '/admin/dashboard' : '/student/home')
    }
    return next()
  }

  if (to.meta.requiresAuth && !token) return next('/login')
  if (to.meta.role === 'admin' && role !== 'admin') return next('/login')
  next()
})

export default router
