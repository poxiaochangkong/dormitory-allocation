// src/utils/request.js
// Axios wrapper with auth token and unified response handling.
import axios from 'axios'
import { ElMessage } from 'element-plus'

const request = axios.create({
  baseURL: '/api',
  timeout: 30000,
  headers: { 'Content-Type': 'application/json' }
})

// Request interceptor: attach token from localStorage
request.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => Promise.reject(error)
)

// Response interceptor: unwrap unified response format {code, message, data}
request.interceptors.response.use(
  (response) => {
    const res = response.data

    // If backend returns unified format with code field
    if (res && typeof res === 'object' && 'code' in res) {
      if (res.code === 200 || res.code === 0) {
        return res.data
      } else {
        const msg = res.message || 'Request failed'
        ElMessage.error(msg)
        return Promise.reject(new Error(msg))
      }
    }

    // If response is already the data itself (no wrapper)
    return res
  },
  (error) => {
    const msg = error.response?.data?.message || error.message || 'Network error'
    ElMessage.error(msg)
    return Promise.reject(error)
  }
)

export default request