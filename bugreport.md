# Bug Report — Dormitory Allocation System

**Generated:** 2026-06-01  
**Method:** Static code analysis (manual review of all source files)

---

## Summary

| Severity | Count |
|----------|-------|
| 🔴 Critical | 5 |
| 🟠 High | 6 |
| 🟡 Medium | 8 |
| 🔵 Low | 5 |

---

## 🔴 Critical Bugs

### C1. Authentication bypass on most API endpoints
- **File:** `backend/src/server/main.cpp`
- **Description:** The original `server/main.cpp` did not validate Bearer tokens on any endpoint except the basic structure was missing. All student endpoints (questionnaire submit, scene submit, get info, get match result) and admin endpoints (import students, create task, run task, etc.) had no authentication — any request was accepted without a token.
- **Impact:** Any user could impersonate any other user, access admin functions, or submit data on behalf of others.
- **Status:** ✅ Fixed — Added `AuthenticateRequest()` helper that validates Bearer tokens against the database, with userId/role verification on all protected endpoints.

### C2. SQL Injection via string concatenation
- **Files:** `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`, `server/main.cpp`
- **Description:** All SQL queries are built via string concatenation with user input. While there is an `Escape()` function that handles single quotes, it does not protect against all SQL injection vectors (e.g., backticks, null bytes, multi-byte character attacks).
- **Impact:** Potential database compromise.
- **Status:** ⚠️ Partially mitigated — `Escape()` covers the most common vector (single quotes). For full protection, prepared statements with parameter binding should be used. This is a significant refactoring effort.

### C3. Token generated with `std::rand()` — not cryptographically secure
- **Files:** `StudentService.cpp`, `AdminService.cpp`
- **Description:** Authentication tokens are generated using `std::rand()` seeded with timestamp milliseconds. `std::rand()` is not a cryptographically secure PRNG; token values can be predicted.
- **Impact:** Session hijacking via predictable tokens.
- **Status:** ⚠️ Not fixed — Should use a CSPRNG (e.g., `std::random_device` with `std::mt19937`, or platform-specific APIs like `BCryptGenRandom` on Windows).

### C4. `AdjustResult` only swaps dorm_id, does not update roommate_ids
- **File:** `AdminService.cpp` — `AdjustResult()`
- **Description:** When admin adjusts allocation results (swaps two students), only the `dorm_id` field is updated. The `roommate_ids` field in each `match_result` row is NOT recalculated, leading to stale/incorrect roommate data after adjustment.
- **Impact:** Data inconsistency — roommate lists become wrong after adjustments.
- **Status:** ⚠️ Not fixed — After swapping dorm_id, need to recalculate roommate_ids for all affected results in both source and target rooms.

### C5. `ImportStudents` uses `ON DUPLICATE KEY UPDATE` but INSERT always generates new `user_id`
- **File:** `AdminService.cpp` — `ImportStudents()`
- **Description:** Each import generates a new `user_id` via `GenId("u_")`. If a student with the same `student_no` already exists, the INSERT fails on the UNIQUE constraint, and `ON DUPLICATE KEY UPDATE` updates the existing row. However, the count is incremented even if the student already existed (it counts `count++` for every non-empty student_no).
- **Impact:** Misleading import count reported to admin.
- **Status:** ⚠️ Not fixed.

---

## 🟠 High Severity Bugs

### H1. Frontend questionnaire uses letter codes (A/B/C/D) but backend expects different formats
- **Files:** `QuestionnaireView.vue` vs `StudentService.cpp`
- **Description:** The frontend sends letter codes like `q02_sleepTime: "A"`, `q06_hygiene: "B"`, `p22_socialEnergy: "A"`. The backend's questionnaire table stores `sleep_schedule` (string like "early"/"normal"/"late"), `hygiene_level` (int 1-5), `social_preference` (int 1-5). Without the format mapping logic, data would be stored incorrectly.
- **Impact:** Questionnaire data would be gibberish in the database, making the matching algorithm useless.
- **Status:** ✅ Fixed — Added mapping functions (`MapSleepSchedule`, `MapToLevel`, `MapSocialPreference`) in `StudentService::SubmitQuestionnaire()`.

### H2. `GreedyAssign` uses `dorm_idx` variable incorrectly
- **File:** `MatchEngine.cpp` — `GreedyAssign()`
- **Description:** The loop variable `dorm_idx` is incremented at the end of each iteration, but the loop is `for (const auto &dorm : dorms)` which already iterates through dorms. The `dorm_idx` is used in a redundant bounds check `if (dorm_idx >= dorms.size()) break;` which would never trigger since the for loop already handles iteration.
- **Impact:** No functional bug, but the `dorm_idx` is misleading dead code. The real issue is that if there are more dorms than student groups, the loop correctly stops when no more pairs are available.
- **Status:** ⚠️ Minor — Dead code, no functional impact.

### H3. `MatchEngine::Match()` method is incomplete
- **File:** `MatchEngine.cpp`
- **Description:** The header declares `Match()` but the implementation in the .cpp file is `ExecuteAllocation()` which is a static method. The `Match()` instance method appears to be unimplemented or has been superseded.
- **Impact:** If code tries to call `Match()` on an instance, it will get a linker error.
- **Status:** ⚠️ Not fixed — The `Match()` declaration in the header should be removed or implemented.

### H4. Frontend admin login sends `username`/`password`, backend expects `studentNo`/`password`
- **Files:** `AdminLogin.vue` vs `server/main.cpp`
- **Description:** The admin login frontend sends `{ username, password }` in the request body. The backend reads `body.value("studentNo", "")`. Since the key doesn't match, `studentNo` will always be empty, and the login will always fail.
- **Impact:** Admin login is completely broken — admin cannot log in.
- **Status:** ⚠️ Not fixed — Either the frontend should send `studentNo` instead of `username`, or the backend should read `username`. Recommendation: change frontend to send `studentNo`.

### H5. CORS headers missing on error responses
- **File:** `server/main.cpp`
- **Description:** When `JsonError()` is called, CORS headers are not set. The frontend will fail to read error responses due to browser CORS policy.
- **Impact:** Frontend cannot properly handle API errors — the browser blocks the response.
- **Status:** ✅ Fixed — Added `SetCors(res)` call before every `JsonError()` in all route handlers.

### H6. `SaveAllocationRule` stores raw JSON in SQL without proper escaping of nested quotes
- **File:** `AdminService.cpp` — `SaveAllocationRule()`
- **Description:** The `rule_json` string is escaped with `Escape()` (single quotes doubled), but the JSON itself may contain backslashes or other SQL-significant characters.
- **Impact:** Potential SQL error or injection when saving complex allocation rules.
- **Status:** ⚠️ Not fixed.

---

## 🟡 Medium Severity Bugs

### M1. `GenerateToken()` calls `std::srand()` — affects global random state
- **Files:** `StudentService.cpp`, `AdminService.cpp`
- **Description:** `GenerateToken()` calls `std::srand()` which reseeds the global RNG. If two login requests arrive close together, they could get the same seed and thus the same token.
- **Impact:** Potential token collision, allowing session hijacking.
- **Status:** ⚠️ Not fixed.

### M2. `GenId()` uses milliseconds + rand — potential ID collision
- **Files:** `StudentService.cpp`, `AdminService.cpp`, `MatchEngine.cpp`
- **Description:** IDs are generated from timestamp milliseconds + `rand() % 10000`. Under concurrent requests or rapid successive calls, IDs can collide.
- **Impact:** Data corruption — duplicate primary key errors or silent data overwrites.
- **Status:** ⚠️ Not fixed — Should use UUID or database auto-increment.

### M3. Frontend `AdminDashboard.vue` calls undefined function `refreshTaskList()`
- **File:** `AdminDashboard.vue`
- **Description:** The template references `refreshTaskList()` but this function is not defined in the component's methods.
- **Impact:** Runtime JavaScript error when the button is clicked. The task list cannot be refreshed.
- **Status:** ⚠️ Not fixed — Need to add `refreshTaskList` method or rename to `fetchTasks`.

### M4. Frontend `QuestionnaireView.vue` — `nightOwls` field not mapped
- **File:** `QuestionnaireView.vue`
- **Description:** The questionnaire form includes a `nightOwls` field, but the backend `SubmitQuestionnaire` does not process this field. It gets stored in `raw_answers` JSON but is never used by the matching algorithm.
- **Impact:** User preference data collected but not utilized in matching.

### M5. Frontend immersive scene — `ImmersiveScene.vue` referenced in router but file missing
- **File:** `frontend-matching/src/router/index.js`
- **Description:** The router imports `ImmersiveScene` from `../views/ImmersiveScene.vue`, but this file does not exist in the project.
- **Impact:** Runtime error when navigating to `/scene` route — white screen.
- **Status:** ⚠️ Not fixed — Need to create `ImmersiveScene.vue`.

### M6. `GetMatchResult` derives fake sub-scores
- **File:** `StudentService.cpp` — `GetMatchResult()`
- **Description:** The method returns `hygieneConsistencyScore` and `scheduleOverlapScore` that are derived by arbitrary formulas (`sim_score * 0.9 + 0.05` and `sim_score * 0.85 + 0.1`). These are not real measurements from the algorithm.
- **Impact:** Misleading data displayed in the frontend radar chart.

### M7. `ListTasks` and `ListDormitories` have no auth
- **File:** `server/main.cpp`
- **Description:** GET `/api/admin/tasks` and GET `/api/admin/dormitories` have no authentication check, while other admin endpoints do.
- **Impact:** Anyone can list all tasks and dormitories without logging in.
- **Status:** ⚠️ Not fixed — Should add auth checks for consistency.

### M8. `ExportResult` CSV doesn't escape commas in field values
- **File:** `AdminService.cpp` — `ExportResult()`
- **Description:** If any field (college, major, etc.) contains a comma, the CSV output will be malformed.
- **Impact:** Invalid CSV file that can't be properly parsed by Excel or other tools.
- **Status:** ⚠️ Not fixed.

---

## 🔵 Low Severity Issues

### L1. No password strength validation on registration
- **File:** `StudentService.cpp` — `Register()`
- **Description:** Any password is accepted, including empty string (only blocked by the `password.empty()` check).

### L2. Frontend API base URL hardcoded to `localhost:8080`
- **File:** `frontend-matching/src/api/index.js`
- **Description:** The axios baseURL is hardcoded. It should use an environment variable.
- **Status:** Note — this is expected for development, but needs configuration for production.

### L3. No rate limiting on login endpoints
- **File:** `server/main.cpp`
- **Description:** No brute-force protection on `/api/student/login` or `/api/admin/login`.

### L4. `db_mutex` blocks all concurrent requests
- **File:** `server/main.cpp`
- **Description:** A single mutex guards all database access. Under load, this serializes all requests and becomes a bottleneck.
- **Impact:** Poor performance under concurrent load.
- **Status:** Note — acceptable for a small deployment, but should use connection pooling for production.

### L5. Token never expires
- **File:** `StudentService.cpp`, `AdminService.cpp`
- **Description:** Tokens have no expiration mechanism. Once issued, they remain valid indefinitely unless the user logs in again (which generates a new token, invalidating the old one implicitly).

---

## Frontend-Backend Interface Issues

| # | Endpoint | Frontend Sends | Backend Expects | Status |
|---|----------|---------------|-----------------|--------|
| 1 | Admin Login | `{ username, password }` | `{ studentNo, password }` | ❌ Mismatch |
| 2 | Questionnaire Submit | Letter codes (A/B/C/D) | String/int values | ✅ Fixed (mapping added) |
| 3 | Questionnaire Status | GET with token header | Token auth | ✅ Fixed |
| 4 | Scene Submit | Scene data JSON | JSON with userId | ✅ Fixed |
| 5 | Match Result | GET with token header | Token auth | ✅ Fixed |
| 6 | Admin Task Create | `{ taskName, ... }` | `{ taskName, ... }` | ✅ Match |
| 7 | Admin Import | JSON array of students | JSON array | ✅ Match |

---

## Recommendations (Priority Order)

1. **Fix H4 (Admin login field name mismatch)** — This blocks all admin functionality
2. **Fix C4 (AdjustResult stale roommate_ids)** — Data corruption after adjustments
3. **Fix C3 (Token security)** — Use CSPRNG for token generation
4. **Fix M5 (Missing ImmersiveScene.vue)** — Route will crash
5. **Fix M3 (refreshTaskList undefined)** — Admin UI broken
6. **Implement prepared statements** — Replace string concatenation SQL (C2)
7. **Add proper UUID generation** — Replace timestamp+rand ID generation (M2)