// utils/request.js
// 这个文件封装了 axios 请求，包含了请求和响应的拦截器
import axios from 'axios'
import { ElMessage } from 'element-plus'

// 创建 axios 实例
const service = axios.create({
  baseURL: '/api', // 统一的接口前缀
  timeout: 5000 // 请求超时时间 5秒
})

// 请求拦截器 (Request Interceptor)
service.interceptors.request.use(
  config => {
    // 这里可以加上 Token，装得像一个真实的登录系统
    //Token是什么？它是一个字符串，通常由服务器生成，用于标识用户的身份和权限。它可以存储在浏览器的 localStorage 或 sessionStorage 中，并在每次请求时发送给服务器，以验证用户的身份。
    const token = localStorage.getItem('token')
    if (token) {
      config.headers['Authorization'] = 'Bearer ' + token
    }
    return config
  },
  error => {
    console.error('请求发送异常', error)
    return Promise.reject(error)
  },
)

// 响应拦截器 (Response Interceptor)
service.interceptors.response.use(
  response => {
    const res = response.data
    // 假设后端规定的成功状态码是 200
    if (res.code !== 200) {
      ElMessage.error(res.message || '系统内部异常')
      return Promise.reject(new Error(res.message || 'Error'))
    } else {
      return res.data
    }
  },
  error => {
    ElMessage.error('网络请求失败，请检查后端服务是否启动')
    return Promise.reject(error)
  }
)

export default service