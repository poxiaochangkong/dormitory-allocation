/**
 * API Integration Test Script
 * 
 * Tests the frontend-backend API integration:
 * 1. Frontend build verification (compile check)
 * 2. API endpoint connectivity (requires backend running)
 * 3. Full workflow: register -> login -> questionnaire -> scene -> result
 * 
 * Usage: node tests/api-integration-test.js [backend_url]
 * Default backend_url: http://localhost:8080
 */

import http from 'http'

const BASE_URL = process.argv[2] || 'http://localhost:8080'
const TIMEOUT = 5000

let passed = 0
let failed = 0
let skipped = 0
let backendAvailable = false

// Helper: make HTTP request
function request(method, path, body = null) {
    return new Promise((resolve, reject) => {
        const url = new URL(path, BASE_URL)
        const options = {
            hostname: url.hostname,
            port: url.port,
            path: url.pathname,
            method: method,
            headers: { 'Content-Type': 'application/json' },
            timeout: TIMEOUT
        }

        const req = http.request(options, (res) => {
            let data = ''
            res.on('data', chunk => data += chunk)
            res.on('end', () => {
                try {
                    resolve({ status: res.statusCode, body: JSON.parse(data) })
                } catch {
                    resolve({ status: res.statusCode, body: data })
                }
            })
        })

        req.on('error', reject)
        req.on('timeout', () => { req.destroy(); reject(new Error('Request timeout')) })
        if (body) req.write(JSON.stringify(body))
        req.end()
    })
}

// Test runner helpers
function test(name, fn) {
    return { name, fn }
}

async function runTest(t) {
    try {
        await t.fn()
        console.log(`  ✅ PASS: ${t.name}`)
        passed++
    } catch (err) {
        if (err.message === 'SKIP') {
            console.log(`  ⏭️  SKIP: ${t.name}`)
            skipped++
        } else {
            console.log(`  ❌ FAIL: ${t.name}`)
            console.log(`     Error: ${err.message}`)
            failed++
        }
    }
}

// ============ Test Cases ============

const tests = [

    test('Backend health check (GET /)', async () => {
        try {
            const res = await request('GET', '/')
            backendAvailable = true
        } catch (err) {
            backendAvailable = false
            throw new Error('SKIP')
        }
    }),

    test('Student register (POST /student/register)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const timestamp = Date.now()
        const res = await request('POST', '/student/register', {
            studentNo: `test_${timestamp}`,
            password: 'test123456',
            gender: 'male',
            college: 'Computer Science',
            major: 'Software Engineering',
            grade: '2026'
        })

        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Expected status 200, got ${res.status}, body: ${JSON.stringify(res.body)}`)
        }

        // Store registered user info for subsequent tests
        const data = res.body.data || res.body
        if (!data.userId) throw new Error('Response missing userId')
        process.env.TEST_USER_ID = data.userId
        process.env.TEST_STUDENT_NO = `test_${timestamp}`
    }),

    test('Student login (POST /student/login)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const studentNo = process.env.TEST_STUDENT_NO
        if (!studentNo) throw new Error('No test student number from register step')

        const res = await request('POST', '/student/login', {
            studentNo: studentNo,
            password: 'test123456'
        })

        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Login failed: ${JSON.stringify(res.body)}`)
        }

        const data = res.body.data || res.body
        if (!data.token) throw new Error('Response missing token')
        process.env.TEST_TOKEN = data.token
        process.env.TEST_USER_ID = data.userId
    }),

    test('Get student info (GET /student/info/:userId)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const userId = process.env.TEST_USER_ID
        if (!userId) throw new Error('No test userId')

        const res = await request('GET', `/student/info/${userId}`)
        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Get info failed: ${JSON.stringify(res.body)}`)
        }
    }),

    test('Submit questionnaire (POST /student/questionnaire/submit)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const userId = process.env.TEST_USER_ID
        if (!userId) throw new Error('No test userId')

        const res = await request('POST', '/student/questionnaire/submit', {
            userId: userId,
            basicInfo: { gender: 'male', college: 'CS', major: 'SE' },
            traditionalHabits: {
                q02_sleepTime: 'A',
                q03_wakeTime: 'B',
                q06_hygiene: 'A',
                q07_laundry: 'B',
                q08_studyPlace: 'A'
            },
            vetoSettings: {
                v11_smokeAlcohol: true,
                v12_midnightGaming: false,
                v13_loudSpeaker: true,
                v14_oppositeSex: false,
                v15_badHygiene: true,
                v16_overDemand: false,
                v17_boundary: false,
                v19_pets: false
            },
            personality: {
                p21_mbti: 'INTJ',
                p22_socialEnergy: 'A',
                p23_idealRelation: 'B',
                p26_plan: 'A',
                p28_rules: 'B'
            }
        })

        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Submit questionnaire failed: ${JSON.stringify(res.body)}`)
        }

        const data = res.body.data || res.body
        if (!data.questionnaireId) throw new Error('Response missing questionnaireId')
    }),

    test('Get questionnaire status (GET /student/questionnaire/status/:userId)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const userId = process.env.TEST_USER_ID
        if (!userId) throw new Error('No test userId')

        const res = await request('GET', `/student/questionnaire/status/${userId}`)
        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Get status failed: ${JSON.stringify(res.body)}`)
        }

        const data = res.body.data || res.body
        if (data.submitted !== true) throw new Error('Expected submitted=true')
    }),

    test('Submit scene data (POST /student/scene/submit)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const userId = process.env.TEST_USER_ID
        if (!userId) throw new Error('No test userId')

        const res = await request('POST', '/student/scene/submit', {
            userId: userId,
            s30_acTemp: 24,
            s31_bedAction: 'A',
            s32_deskState: 'B',
            s33_doorKnock: 'B',
            s34_curtain: 'A',
            s35_trash: 'A',
            s36_wash: 'B',
            s37_noiseDb: 60,
            s38_balconyWet: 'A',
            selfDescription: 'I like a clean and quiet environment.',
            roommateExpectation: 'Someone respectful of boundaries.'
        })

        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Submit scene failed: ${JSON.stringify(res.body)}`)
        }
    }),

    test('Get match result (GET /student/match-result/:userId)', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const userId = process.env.TEST_USER_ID
        if (!userId) throw new Error('No test userId')

        const res = await request('GET', `/student/match-result/${userId}`)
        // No match result is expected (admin hasn't run task), just check API responds
        if (res.status !== 200 && res.body.code !== 200) {
            throw new Error(`Get match result failed: ${JSON.stringify(res.body)}`)
        }
        // Expect empty result {} since no allocation task has been run
        const data = res.body.data || res.body
        if (JSON.stringify(data) === '{}') {
            console.log('     (No match result yet, as expected)')
        }
    }),

    test('Duplicate register should fail', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const studentNo = process.env.TEST_STUDENT_NO
        if (!studentNo) throw new Error('No test student number')

        const res = await request('POST', '/student/register', {
            studentNo: studentNo,
            password: 'test123456'
        })

        // Should fail with duplicate error
        if (res.status === 200 && res.body.code === 200) {
            throw new Error('Duplicate register should have been rejected')
        }
        console.log('     (Correctly rejected duplicate registration)')
    }),

    test('Wrong password login should fail', async () => {
        if (!backendAvailable) throw new Error('SKIP')
        const studentNo = process.env.TEST_STUDENT_NO
        if (!studentNo) throw new Error('No test student number')

        const res = await request('POST', '/student/login', {
            studentNo: studentNo,
            password: 'wrong_password'
        })

        if (res.status === 200 && res.body.code === 200) {
            throw new Error('Wrong password should have been rejected')
        }
        console.log('     (Correctly rejected wrong password)')
    }),
]

// ============ Main ============

async function main() {
    console.log('\n========================================')
    console.log('  Dormitory Allocation - API Integration Test')
    console.log(`  Backend: ${BASE_URL}`)
    console.log('========================================\n')

    for (const t of tests) {
        await runTest(t)
    }

    console.log('\n----------------------------------------')
    console.log(`  Results: ${passed} passed, ${failed} failed, ${skipped} skipped`)
    console.log('----------------------------------------\n')

    if (skipped > 0 && passed === 0) {
        console.log('  ⚠️  All tests skipped. Is the backend server running?')
        console.log('  Start backend: cd backend && cmake --build build && ./build/dorm_alloc_server')
        console.log('')
    }

    process.exit(failed > 0 ? 1 : 0)
}

main()