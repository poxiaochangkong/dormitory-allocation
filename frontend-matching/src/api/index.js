// src/api/index.js
// 这里定义了与后端 API 交互的函数，封装了请求细节，方便在组件中调用
import request from '../utils/request'

// 提交学生问卷数据
//url是后端接口地址，method是请求方法，data是请求体数据
export function submitQuestionnaire(data) {
  return request({
    url: '/student/questionnaire/submit',
    method: 'post',
    data: data
  })
}

// 获取分配结果
export function getMatchResult(userId) {
  return request({
    url: `/student/match-result/${userId}`,
    method: 'get'
  })
}

// (预留) 管理员：人工调整宿舍
export function adjustDormitory(data) {
  return request({
    url: '/admin/allocation/task/adjust',
    method: 'post',
    data: data
  })
}