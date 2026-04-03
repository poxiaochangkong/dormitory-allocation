//创建路由实例并传递 `routes` 配置
import { createRouter, createWebHistory } from 'vue-router'
// 暂时先只定义登录页，稍后我们建这个文件
const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', name: 'Login', component: () => import('../views/Login.vue') }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router