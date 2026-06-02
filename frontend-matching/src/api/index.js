// src/api/index.js
// All API functions for communicating with the backend.
import request from '../utils/request'

// ==================== Student APIs ====================

// Student login
export function studentLogin(studentNo, password) {
  return request({
    url: '/student/login',
    method: 'post',
    data: { studentNo, password }
  })
}

// Student register
export function studentRegister(data) {
  return request({
    url: '/student/register',
    method: 'post',
    data: data
  })
}

// Get student info
export function getStudentInfo(userId) {
  return request({
    url: `/student/info/${userId}`,
    method: 'get'
  })
}

// Get questionnaire status
export function getQuestionnaireStatus(userId) {
  return request({
    url: `/student/questionnaire/status/${userId}`,
    method: 'get'
  })
}

// Submit questionnaire data (baseline + veto + personality)
export function submitQuestionnaire(data) {
  return request({
    url: '/student/questionnaire/submit',
    method: 'post',
    data: data
  })
}

// Submit immersive scene data
export function submitSceneData(data) {
  return request({
    url: '/student/scene/submit',
    method: 'post',
    data: data
  })
}

// Get match result for a student
export function getMatchResult(userId) {
  return request({
    url: `/student/match-result/${userId}`,
    method: 'get'
  })
}

// ==================== Admin APIs ====================

// Admin login
export function adminLogin(studentNo, password) {
  return request({
    url: '/admin/login',
    method: 'post',
    data: { studentNo, password }
  })
}

// Import students
export function importStudents(data) {
  return request({
    url: '/admin/students/import',
    method: 'post',
    data: data
  })
}

// List all tasks
export function listTasks() {
  return request({
    url: '/admin/tasks',
    method: 'get'
  })
}

// Create a new allocation task
export function createTask(data) {
  return request({
    url: '/admin/allocation/task/create',
    method: 'post',
    data: data
  })
}

// Run a task (execute matching algorithm)
export function runTask(taskId) {
  return request({
    url: `/admin/allocation/task/run/${taskId}`,
    method: 'post'
  })
}

// Get task result
export function getTaskResult(taskId) {
  return request({
    url: `/admin/allocation/task/result/${taskId}`,
    method: 'get'
  })
}

// Admin adjust dormitory assignment
export function adjustDormitory(data) {
  return request({
    url: '/admin/allocation/task/adjust',
    method: 'post',
    data: data
  })
}

// Export task result as CSV — use fetch to avoid axios JSON parsing
export async function exportTaskResult(taskId) {
  const resp = await fetch(`/api/admin/allocation/task/export/${taskId}`, {
    headers: { 'Authorization': 'Bearer ' + localStorage.getItem('token') }
  })
  if (!resp.ok) throw new Error('Export failed')
  return resp.text()
}

// Delete a task and its results
export function deleteTask(taskId) {
  return request({
    url: `/admin/allocation/task/${taskId}`,
    method: 'delete'
  })
}

// List all users (admin only, requires auth)
export function listUsers() {
  return request({
    url: '/admin/users',
    method: 'get'
  })
}

// List all dormitories
export function listDormitories() {
  return request({
    url: '/admin/dormitories',
    method: 'get'
  })
}

// Add a dormitory
export function addDormitory(data) {
  return request({
    url: '/admin/dormitories',
    method: 'post',
    data: data
  })
}

// Save allocation rule
export function saveAllocationRule(data) {
  return request({
    url: '/admin/allocation/rule/save',
    method: 'post',
    data: data
  })
}

// Delete a user (admin cannot delete self or other admins)
export function deleteUser(userId) {
  return request({
    url: `/admin/users/${userId}`,
    method: 'delete'
  })
}

// Transfer admin role to another user
export function transferAdmin(targetUserId) {
  return request({
    url: '/admin/transfer',
    method: 'post',
    data: { targetUserId }
  })
}

// Get algorithm audit log for a task
export function getTaskAudit(taskId) {
  return request({
    url: `/admin/allocation/task/audit/${taskId}`,
    method: 'get'
  })
}
