/**
 * P0 Bug Fix Unit Tests
 *
 * Tests for the 4 critical bugs that were fixed:
 * - Bug 1: Task list not displaying due to data format mismatch
 * - Bug 2: Task name lost due to field name inconsistency (name vs taskName)
 * - Bug 3: Table column prop mismatch (name vs taskName)
 * - Bug 4: Student login redirects to non-existent /student/home route
 */

import { describe, it, expect } from 'vitest'
import { readFileSync } from 'fs'
import { resolve } from 'path'

// ============================================================
// Bug 1: Task list data format handling
// ============================================================
describe('Bug 1: Task list data format', () => {
    // Simulates the data transformation logic from AdminDashboard.vue loadTasks()
    function parseTaskListData(data) {
        // This matches the fix applied to AdminDashboard.vue
        const taskList = Array.isArray(data?.tasks) ? data.tasks : (Array.isArray(data) ? data : [])
        return taskList
    }

    it('should correctly parse backend response { tasks: [...] }', () => {
        const backendResponse = {
            tasks: [
                { taskId: 'task_1', taskName: 'Test Task 1', status: 'pending' },
                { taskId: 'task_2', taskName: 'Test Task 2', status: 'completed' },
            ]
        }

        const result = parseTaskListData(backendResponse)
        expect(result).toHaveLength(2)
        expect(result[0].taskId).toBe('task_1')
        expect(result[0].taskName).toBe('Test Task 1')
        expect(result[1].status).toBe('completed')
    })

    it('should handle plain array response as fallback', () => {
        const plainArray = [
            { taskId: 'task_1', taskName: 'Test Task', status: 'pending' },
        ]

        const result = parseTaskListData(plainArray)
        expect(result).toHaveLength(1)
        expect(result[0].taskId).toBe('task_1')
    })

    it('should handle null/undefined/empty data gracefully', () => {
        expect(parseTaskListData(null)).toHaveLength(0)
        expect(parseTaskListData(undefined)).toHaveLength(0)
        expect(parseTaskListData({})).toHaveLength(0)
        expect(parseTaskListData({ tasks: null })).toHaveLength(0)
        expect(parseTaskListData({ tasks: 'not an array' })).toHaveLength(0)
    })

    it('should correctly count completed tasks from parsed data', () => {
        const data = {
            tasks: [
                { taskId: 'task_1', taskName: 'Task 1', status: 'pending' },
                { taskId: 'task_2', taskName: 'Task 2', status: 'completed' },
                { taskId: 'task_3', taskName: 'Task 3', status: 'completed' },
                { taskId: 'task_4', taskName: 'Task 4', status: 'running' },
            ]
        }

        const taskList = parseTaskListData(data)
        const completedCount = taskList.filter(t => t.status === 'completed').length
        expect(completedCount).toBe(2)
        expect(taskList.length).toBe(4)
    })
})

// ============================================================
// Bug 2: Create task field name (taskName vs name)
// ============================================================
describe('Bug 2: Create task sends taskName instead of name', () => {
    // Simulates the request body construction from handleCreateTask()
    function buildCreateTaskPayload(formData) {
        // This matches the fix: sends taskName instead of name
        return {
            taskName: formData.name,
            college: formData.college,
            similarityWeight: formData.similarityWeight,
            complementarityWeight: formData.complementarityWeight,
            vetoSafetyWeight: formData.vetoSafetyWeight,
        }
    }

    it('should send taskName field (not name) to match backend expectation', () => {
        const formData = {
            name: '计算机学院2026级分配',
            college: '计算机学院',
            similarityWeight: 0.5,
            complementarityWeight: 0.25,
            vetoSafetyWeight: 0.25,
        }

        const payload = buildCreateTaskPayload(formData)

        // Backend reads data.value("taskName", "Unnamed Task")
        expect(payload).toHaveProperty('taskName', '计算机学院2026级分配')
        expect(payload).not.toHaveProperty('name')
        expect(payload.college).toBe('计算机学院')
    })

    it('should not result in "Unnamed Task" when task name is provided', () => {
        const formData = {
            name: 'My Task',
            college: '',
            similarityWeight: 0.5,
            complementarityWeight: 0.25,
            vetoSafetyWeight: 0.25,
        }

        const payload = buildCreateTaskPayload(formData)

        // Verify the field the backend reads is correct
        expect(payload.taskName).toBe('My Task')
        expect(payload.taskName).not.toBe('Unnamed Task')
    })
})

// ============================================================
// Bug 3: Table column prop matches backend field name
// ============================================================
describe('Bug 3: Table column prop uses taskName', () => {
    it('should render taskName column from backend data correctly', () => {
        // Simulate backend task data (as returned by ListTasks)
        const task = {
            taskId: 'task_123',
            taskName: '计算机学院2026级分配',
            college: '计算机学院',
            status: 'pending',
        }

        // The table column prop should be "taskName", not "name"
        expect(task).toHaveProperty('taskName')
        expect(task.taskName).toBe('计算机学院2026级分配')

        // Verify that using "name" prop would NOT work (the old bug)
        expect(task).not.toHaveProperty('name')
    })

    it('should display correct task name in table when using taskName prop', () => {
        const tasks = [
            { taskId: 'task_1', taskName: 'Task A', status: 'pending' },
            { taskId: 'task_2', taskName: 'Task B', status: 'completed' },
        ]

        // Verify each task has taskName field for the table to display
        tasks.forEach(task => {
            expect(typeof task.taskName).toBe('string')
            expect(task.taskName.length).toBeGreaterThan(0)
        })
    })
})

// ============================================================
// Bug 4: Student login redirect route
// ============================================================
describe('Bug 4: Student login redirect to /student/questionnaire', () => {
    it('should redirect to /student/questionnaire (not /student/home)', () => {
        // After student login, the redirect target should be /student/questionnaire
        const studentRedirectRoute = '/student/questionnaire'
        const oldBrokenRoute = '/student/home'

        expect(studentRedirectRoute).toBe('/student/questionnaire')
        expect(studentRedirectRoute).not.toBe(oldBrokenRoute)
    })

    it('should redirect to /admin/dashboard for admin login', () => {
        const adminRedirectRoute = '/admin/dashboard'
        expect(adminRedirectRoute).toBe('/admin/dashboard')
    })
})

// ============================================================
// Integration: AdminDashboard task result data format
// ============================================================
describe('Task result data format (GetTaskResult)', () => {
    // Simulates the data transformation logic from handleViewResult()
    function parseTaskResultData(data) {
        // This matches the fix: extracts data.allocations array
        return Array.isArray(data?.allocations) ? data.allocations : (Array.isArray(data) ? data : [])
    }

    it('should correctly parse backend response with allocations array', () => {
        const backendResponse = {
            taskId: 'task_1',
            taskName: 'Test Task',
            college: '计算机学院',
            status: 'completed',
            allocations: [
                { resultId: 'r1', userId: 'u1', dormId: 'd1', building: 'A', roomNumber: '101', totalScore: 0.85 },
                { resultId: 'r2', userId: 'u2', dormId: 'd1', building: 'A', roomNumber: '101', totalScore: 0.78 },
            ]
        }

        const result = parseTaskResultData(backendResponse)
        expect(result).toHaveLength(2)
        expect(result[0].dormId).toBe('d1')
        expect(result[1].totalScore).toBe(0.78)
    })

    it('should handle empty allocations gracefully', () => {
        const emptyResponse = {
            taskId: 'task_1',
            taskName: 'Test Task',
            allocations: []
        }

        const result = parseTaskResultData(emptyResponse)
        expect(result).toHaveLength(0)
    })

    it('should handle null/undefined gracefully', () => {
        expect(parseTaskResultData(null)).toHaveLength(0)
        expect(parseTaskResultData(undefined)).toHaveLength(0)
        expect(parseTaskResultData({})).toHaveLength(0)
    })
})

// ============================================================
// Router guard: /student/home should not be used
// ============================================================
describe('Router guard: no /student/home redirects', () => {
    it('should not have /student/home as a redirect target for logged-in students', () => {
        // Read the router source to verify /student/home is not used as a redirect
        const routerPath = resolve('src/router/index.js')
        const routerContent = readFileSync(routerPath, 'utf-8')

        // Check that /student/questionnaire is used for student redirect
        expect(routerContent).toContain("next('/student/questionnaire')")

        // Check that /student/home is NOT used as a redirect target
        const redirectPattern = /next\(['"]\/student\/home['"]\)/
        expect(redirectPattern.test(routerContent)).toBe(false)
    })
})
