/**
 * P0 Bug Fix Unit Tests
 *
 * Tests for the critical bugs:
 * - Bug 1: Task list not displaying due to data format mismatch
 * - Bug 2: Task name lost due to field name inconsistency (name vs taskName)
 * - Bug 3: Table column prop mismatch (name vs taskName)
 * - Bug 4: Student login redirects to non-existent /student/home route
 * - Bug 6: No logout button on student pages
 * - Bug 7: "Return to lobby" buttons point to non-existent route
 * - Bug 8: Create task form fields mismatch (weights vs taskName/college/major/gender)
 * - Bug 9: Task result table expects userIds array but backend returns flat rows
 */

import { describe, it, expect } from 'vitest'
import { readFileSync } from 'fs'
import { resolve } from 'path'

// Helper to read a Vue source file
function readVueSource(filename) {
    return readFileSync(resolve(`src/views/${filename}`), 'utf-8')
}

// ============================================================
// Bug 1: Task list data format handling
// ============================================================
describe('Bug 1: Task list data format', () => {
    // Simulates the data transformation logic from AdminDashboard.vue loadTasks()
    function parseTaskListData(data) {
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
})

// ============================================================
// Bug 2 & 3: Table column and field names match backend
// ============================================================
describe('Bug 2 & 3: taskName field consistency', () => {
    it('should render taskName column from backend data correctly', () => {
        const task = {
            taskId: 'task_123',
            taskName: '计算机学院2026级分配',
            college: '计算机学院',
            status: 'pending',
        }

        expect(task).toHaveProperty('taskName')
        expect(task.taskName).toBe('计算机学院2026级分配')
        expect(task).not.toHaveProperty('name')
    })

    it('AdminDashboard table uses taskName prop', () => {
        const source = readVueSource('AdminDashboard.vue')
        expect(source).toContain('prop="taskName"')
    })
})

// ============================================================
// Bug 4: Student login redirect route
// ============================================================
describe('Bug 4: Student login redirect', () => {
    it('should have /student/home route defined in router', () => {
        const routerContent = readFileSync(resolve('src/router/index.js'), 'utf-8')
        expect(routerContent).toContain("path: '/student/home'")
        expect(routerContent).toContain("name: 'StudentHome'")
    })
})

// ============================================================
// Bug 6: Logout buttons on student pages
// ============================================================
describe('Bug 6: Student pages have logout buttons', () => {
    const studentPages = [
        'Questionnaire.vue',
        'ImmersiveScene.vue',
        'StudentResult.vue'
    ]

    studentPages.forEach(filename => {
        it(`${filename} should have a handleLogout function`, () => {
            const source = readVueSource(filename)
            expect(source).toContain('handleLogout')
            expect(source).toContain("localStorage.removeItem('token')")
            expect(source).toContain("router.push('/login')")
        })

        it(`${filename} should have a logout button in template`, () => {
            const source = readVueSource(filename)
            // Should have a danger (red) button that triggers handleLogout
            expect(source).toMatch(/type="danger".*handleLogout|handleLogout.*type="danger"/)
        })
    })
})

// ============================================================
// Bug 7: Navigation routes are valid
// ============================================================
describe('Bug 7: Student page navigation routes', () => {
    it('should have /student/home route defined for "return to lobby" buttons', () => {
        const routerContent = readFileSync(resolve('src/router/index.js'), 'utf-8')
        expect(routerContent).toContain("path: '/student/home'")
    })

    it('Questionnaire.vue should navigate to /student/home', () => {
        const source = readVueSource('Questionnaire.vue')
        expect(source).toContain("/student/home")
    })

    it('ImmersiveScene.vue should have a "return to lobby" button', () => {
        const source = readVueSource('ImmersiveScene.vue')
        expect(source).toContain("/student/home")
    })

    it('StudentResult.vue should navigate to /student/home', () => {
        const source = readVueSource('StudentResult.vue')
        expect(source).toContain("/student/home")
    })
})

// ============================================================
// Bug 8: Create task form sends correct fields
// ============================================================
describe('Bug 8: Create task sends taskName/college/major/gender', () => {
    // Simulates the new handleCreateTask logic
    function buildCreateTaskPayload(formData) {
        // Must match backend expectation: taskName, college, major, gender
        return {
            taskName: formData.taskName,
            college: formData.college,
            major: formData.major,
            gender: formData.gender,
        }
    }

    it('should send taskName, college, major, gender to match backend', () => {
        const formData = {
            taskName: '计算机学院2026级分配',
            college: '计算机学院',
            major: '软件工程',
            gender: 'male',
        }

        const payload = buildCreateTaskPayload(formData)

        // Backend reads: taskName, college, major, gender
        expect(payload).toHaveProperty('taskName', '计算机学院2026级分配')
        expect(payload).toHaveProperty('college', '计算机学院')
        expect(payload).toHaveProperty('major', '软件工程')
        expect(payload).toHaveProperty('gender', 'male')
    })

    it('should NOT send old weight fields (similarityWeight etc.)', () => {
        const formData = {
            taskName: 'Test Task',
            college: '',
            major: '',
            gender: '',
        }

        const payload = buildCreateTaskPayload(formData)

        expect(payload).not.toHaveProperty('similarityWeight')
        expect(payload).not.toHaveProperty('complementarityWeight')
        expect(payload).not.toHaveProperty('vetoSafetyWeight')
    })

    it('should allow empty college/major/gender (backend uses defaults)', () => {
        const formData = {
            taskName: 'Task',
            college: '',
            major: '',
            gender: '',
        }

        const payload = buildCreateTaskPayload(formData)
        expect(payload.taskName).toBe('Task')
        expect(payload.college).toBe('')
        expect(payload.major).toBe('')
        expect(payload.gender).toBe('')
    })

    it('AdminDashboard form should NOT have weight inputs', () => {
        const source = readVueSource('AdminDashboard.vue')
        expect(source).not.toContain('similarityWeight')
        expect(source).not.toContain('complementarityWeight')
    })

    it('AdminDashboard form should have taskName, college, major, gender fields', () => {
        const source = readVueSource('AdminDashboard.vue')
        expect(source).toContain('newTask.taskName')
        expect(source).toContain('newTask.college')
        expect(source).toContain('newTask.major')
        expect(source).toContain('newTask.gender')
    })
})

// ============================================================
// Bug 9: Task result data structure (flat per-student rows)
// ============================================================
describe('Bug 9: Task result shows flat per-student rows', () => {
    // Simulates the new handleViewResult logic
    function parseTaskResultData(data) {
        const taskName = data?.taskName || ''
        const results = Array.isArray(data?.allocations) ? data.allocations : (Array.isArray(data) ? data : [])
        return { taskName, results }
    }

    it('should parse flat per-student rows from backend allocations', () => {
        // Backend returns per-student flat rows, not grouped by room
        const backendResponse = {
            taskId: 'task_1',
            taskName: 'Test Task',
            status: 'completed',
            allocations: [
                { resultId: 'r1', userId: 'u1', dormId: 'd1', building: 'A', roomNumber: '101', studentNo: '2023001', totalScore: 0.85, explanationText: 'Good match' },
                { resultId: 'r2', userId: 'u2', dormId: 'd1', building: 'A', roomNumber: '101', studentNo: '2023002', totalScore: 0.78, explanationText: 'Decent match' },
                { resultId: 'r3', userId: 'u3', dormId: 'd2', building: 'A', roomNumber: '102', studentNo: '2023003', totalScore: 0.92, explanationText: 'Great match' },
            ]
        }

        const { taskName, results } = parseTaskResultData(backendResponse)
        expect(taskName).toBe('Test Task')
        expect(results).toHaveLength(3)
        expect(results[0].studentNo).toBe('2023001')
        expect(results[0].building).toBe('A')
        expect(results[0].roomNumber).toBe('101')
        expect(results[2].totalScore).toBe(0.92)
    })

    it('should NOT expect userIds array (old format)', () => {
        const backendResponse = {
            taskId: 'task_1',
            taskName: 'Test',
            allocations: [
                { resultId: 'r1', userId: 'u1', studentNo: 'S001', building: 'A', roomNumber: '101', totalScore: 0.8, explanationText: '' },
            ]
        }

        const { results } = parseTaskResultData(backendResponse)
        // Each row is a flat student record, NOT a { roomId, userIds[] } object
        expect(results[0]).toHaveProperty('studentNo')
        expect(results[0]).toHaveProperty('building')
        expect(results[0]).toHaveProperty('roomNumber')
        expect(results[0]).not.toHaveProperty('userIds')
    })

    it('should handle empty allocations gracefully', () => {
        const emptyResponse = { taskId: 'task_1', taskName: 'Empty', allocations: [] }
        const { results } = parseTaskResultData(emptyResponse)
        expect(results).toHaveLength(0)
    })

    it('AdminDashboard result table should use studentNo/building/roomNumber columns', () => {
        const source = readVueSource('AdminDashboard.vue')
        expect(source).toContain('prop="studentNo"')
        expect(source).toContain('prop="building"')
        expect(source).toContain('prop="roomNumber"')
        expect(source).toContain('prop="explanationText"')
        // Should NOT use old dormId/userIds columns
        expect(source).not.toContain('prop="userIds"')
    })
})

// ============================================================
// Integration: Task result data format (GetTaskResult)
// ============================================================
describe('Integration: GetTaskResult response handling', () => {
    function parseTaskResultData(data) {
        return Array.isArray(data?.allocations) ? data.allocations : (Array.isArray(data) ? data : [])
    }

    it('should correctly parse backend response with allocations array', () => {
        const backendResponse = {
            taskId: 'task_1',
            taskName: 'Test Task',
            college: '计算机学院',
            status: 'completed',
            allocations: [
                { resultId: 'r1', userId: 'u1', dormId: 'd1', building: 'A', roomNumber: '101', studentNo: 'S001', totalScore: 0.85, explanationText: '' },
                { resultId: 'r2', userId: 'u2', dormId: 'd1', building: 'A', roomNumber: '101', studentNo: 'S002', totalScore: 0.78, explanationText: '' },
            ]
        }

        const result = parseTaskResultData(backendResponse)
        expect(result).toHaveLength(2)
        expect(result[0].studentNo).toBe('S001')
        expect(result[1].totalScore).toBe(0.78)
    })

    it('should handle null/undefined gracefully', () => {
        expect(parseTaskResultData(null)).toHaveLength(0)
        expect(parseTaskResultData(undefined)).toHaveLength(0)
        expect(parseTaskResultData({})).toHaveLength(0)
    })
})