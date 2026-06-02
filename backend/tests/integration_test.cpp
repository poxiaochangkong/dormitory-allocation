// Integration test for the dormitory allocation backend.
// Tests all Service and Algorithm layers against a real MySQL database.
// All test data uses "test_" prefixed IDs and "TEST" prefixed student numbers
// so they can be safely cleaned up after the test run.

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <mysql/jdbc.h>
#include <nlohmann/json.hpp>

#include "infrastructure/auth/CryptoUtil.h"
#include "infrastructure/config/AppConfig.h"
#include "infrastructure/db/DbBootstrapper.h"
#include "infrastructure/db/MySqlClient.h"
#include "service/StudentService.h"
#include "service/AdminService.h"

using MySqlClient = dorm_alloc::infra::db::MySqlClient;
using namespace dorm_alloc::service;
using namespace dorm_alloc::infra::db;
using namespace dorm_alloc::infra::config;

// ---------------------------------------------------------------------------
// Test framework helpers
// ---------------------------------------------------------------------------

static int g_pass = 0;
static int g_fail = 0;
static int g_current_step = 0;

#define TEST_STEP(name)                                   \
    do                                                    \
    {                                                     \
        ++g_current_step;                                 \
        std::cout << "[TEST] Step " << std::setw(2)       \
                  << g_current_step << ": " << std::left  \
                  << std::setw(36) << name << std::flush; \
    } while (0)

#define PASS()                            \
    do                                    \
    {                                     \
        std::cout << "PASS" << std::endl; \
        ++g_pass;                         \
    } while (0)

#define FAIL(msg)                                   \
    do                                              \
    {                                               \
        std::cout << "FAIL" << std::endl;           \
        std::cerr << "  -> " << (msg) << std::endl; \
        ++g_fail;                                   \
    } while (0)

// Cleanup all test data from every table
static void CleanupTestData(MySqlClient &db)
{
    // Delete in reverse FK dependency order
    db.Execute("DELETE FROM match_result WHERE result_id LIKE 'test_%';");
    db.Execute("DELETE FROM match_result WHERE task_id LIKE 'test_%';");
    db.Execute("DELETE FROM allocation_task WHERE task_id LIKE 'test_%';");

    // Find test user IDs by student_no prefix
    std::vector<std::string> test_user_ids;
    try
    {
        auto rs = db.ExecuteQuery("SELECT user_id FROM `user` WHERE student_no LIKE 'TEST%';");
        while (rs->next())
        {
            test_user_ids.push_back(rs->getString("user_id").asStdString());
        }
    }
    catch (const std::exception &)
    {
    }

    // Delete related records for those user IDs
    for (const auto &uid : test_user_ids)
    {
        db.Execute("DELETE FROM match_result WHERE user_id = '" + uid + "';");
        db.Execute("DELETE FROM veto WHERE user_id = '" + uid + "';");
        db.Execute("DELETE FROM open_text_profile WHERE user_id = '" + uid + "';");
        db.Execute("DELETE FROM preference WHERE user_id = '" + uid + "';");
        db.Execute("DELETE FROM questionnaire WHERE user_id = '" + uid + "';");
    }

    // Also delete by user_id LIKE 'test_%' for explicitly renamed IDs
    db.Execute("DELETE FROM veto WHERE user_id LIKE 'test_%';");
    db.Execute("DELETE FROM open_text_profile WHERE user_id LIKE 'test_%';");
    db.Execute("DELETE FROM preference WHERE user_id LIKE 'test_%';");
    db.Execute("DELETE FROM questionnaire WHERE user_id LIKE 'test_%';");
    db.Execute("DELETE FROM dormitory WHERE dorm_id LIKE 'test_%';");

    // Delete users by student_no prefix
    db.Execute("DELETE FROM `user` WHERE student_no LIKE 'TEST%';");
    db.Execute("DELETE FROM `user` WHERE user_id LIKE 'test_%';");

    // Restore admin_001 role in case a failed TransferAdmin left it as student
    try
    {
        db.Execute("UPDATE `user` SET role = 'admin', token = NULL WHERE user_id = 'admin_001';");
    }
    catch (const std::exception &)
    {
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    std::string config_path = "config/config.example.json";
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--config" && i + 1 < argc)
        {
            config_path = argv[i + 1];
            ++i;
        }
    }

    try
    {
        // ====== Step 1: Database connection ======
        TEST_STEP("Database connection");

        auto cfg = LoadFromJsonFile(config_path);
        MySqlClient db(cfg.mysql);
        db.ConnectServer();
        DbBootstrapper::EnsureDatabaseExists(db, cfg.mysql.database);
        DbBootstrapper::EnsureTablesExist(db);

        PASS();

        // ====== Step 2: CryptoUtil SHA-256 unit tests (pure, no DB) ======

        TEST_STEP("SHA-256 empty string");
        {
            std::string hash = dorm_alloc::infra::auth::CryptoUtil::SHA256("");
            if (hash == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
                PASS();
            else
                FAIL("SHA256('') mismatch: " + hash);
        }

        TEST_STEP("SHA-256 'abc'");
        {
            std::string hash = dorm_alloc::infra::auth::CryptoUtil::SHA256("abc");
            if (hash == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
                PASS();
            else
                FAIL("SHA256('abc') mismatch: " + hash);
        }

        TEST_STEP("GenerateSalt unique");
        {
            std::string s1 = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
            std::string s2 = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
            if (s1.size() == 32 && s2.size() == 32 && s1 != s2)
                PASS();
            else
                FAIL("Salts should be 32 hex chars and different");
        }

        TEST_STEP("HashPassword + Verify");
        {
            std::string salt = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
            std::string hash = dorm_alloc::infra::auth::CryptoUtil::HashPassword("mypassword", salt);
            bool ok = dorm_alloc::infra::auth::CryptoUtil::VerifyPassword("mypassword", salt, hash);
            bool bad = dorm_alloc::infra::auth::CryptoUtil::VerifyPassword("wrongpassword", salt, hash);
            if (ok && !bad)
                PASS();
            else
                FAIL("VerifyPassword logic failed");
        }

        // ====== Step 3: Cleanup residual test data ======
        TEST_STEP("Cleanup residual test data");
        CleanupTestData(db);
        PASS();

        // ====== Step 4: Import 12 test students (admin filtered) ======
        TEST_STEP("Import students (admin filtered)");

        nlohmann::json students = nlohmann::json::array();
        const char *colleges[] = {"ComputerScience", "Mathematics"};
        const char *majors[] = {"SoftwareEng", "AppliedMath"};
        const char *genders[] = {"male", "female"};

        int student_idx = 0;
        for (int c = 0; c < 2; ++c)
        {
            for (int g = 0; g < 2; ++g)
            {
                for (int s = 0; s < 3; ++s)
                {
                    std::string sno = "TEST" + std::to_string(student_idx + 1001);
                    nlohmann::json stu;
                    stu["studentNo"] = sno;
                    stu["gender"] = genders[g];
                    stu["college"] = colleges[c];
                    stu["major"] = majors[c];
                    stu["grade"] = "2024";
                    stu["dormType"] = 4;
                    stu["password"] = "test123";
                    stu["role"] = "student";
                    students.push_back(stu);
                    student_idx++;
                }
            }
        }

        // Add one admin user that should be filtered out
        nlohmann::json admin;
        admin["studentNo"] = "TESTADMIN001";
        admin["gender"] = "male";
        admin["college"] = "ComputerScience";
        admin["major"] = "SoftwareEng";
        admin["grade"] = "2024";
        admin["password"] = "admin123";
        admin["role"] = "admin";
        students.push_back(admin);

        auto import_result = AdminService::ImportStudents(db, students.dump());
        auto import_json = nlohmann::json::parse(import_result);
        if (import_json["imported"].get<int>() == 12)
        {
            PASS();
        }
        else
        {
            FAIL("Expected 12 imported (admin filtered), got " + import_result);
        }

        // ====== Step 5: Student login ======
        TEST_STEP("Student login (correct password)");

        try
        {
            auto login_result = StudentService::Login(db, "TEST1001", "test123");
            auto login_json = nlohmann::json::parse(login_result);
            if (login_json.contains("userId") && login_json["role"] == "student")
            {
                PASS();
            }
            else
            {
                FAIL("Unexpected login response: " + login_result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 5b: Student login wrong password ======
        TEST_STEP("Student login (wrong password)");

        try
        {
            StudentService::Login(db, "TEST1001", "wrongpassword");
            FAIL("Expected login to fail but it succeeded");
        }
        catch (const std::exception &)
        {
            PASS(); // Expected to throw
        }

        // ====== Step 6: Seed admin login ======
        TEST_STEP("Seed admin login (admin/admin123)");

        std::string admin_token;
        try
        {
            auto login_result = AdminService::Login(db, "admin", "admin123");
            auto login_json = nlohmann::json::parse(login_result);
            if (login_json.contains("userId") && login_json["role"] == "admin")
            {
                admin_token = login_json.value("token", "");
                PASS();
            }
            else
            {
                FAIL("Unexpected admin login response: " + login_result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 6b: Admin login with student credentials ======
        TEST_STEP("Admin login (student creds)");

        try
        {
            AdminService::Login(db, "TEST1001", "test123");
            FAIL("Expected admin login to fail for student account");
        }
        catch (const std::exception &)
        {
            PASS(); // Expected to throw
        }

        // ====== Step 7: Student Register ======
        TEST_STEP("Student register");

        std::string registered_user_id;
        try
        {
            nlohmann::json reg;
            reg["studentNo"] = "TESTREG001";
            reg["password"] = "regpass123";
            reg["gender"] = "male";
            reg["college"] = "ComputerScience";
            reg["major"] = "SoftwareEng";
            reg["grade"] = "2025";
            reg["dormType"] = 4;

            auto result = StudentService::Register(db, reg.dump());
            auto rj = nlohmann::json::parse(result);
            registered_user_id = rj.value("userId", "");
            if (!registered_user_id.empty() && rj["role"] == "student")
            {
                PASS();
            }
            else
            {
                FAIL("Register response missing userId: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 7b: Register duplicate should fail ======
        TEST_STEP("Register duplicate fails");

        try
        {
            nlohmann::json reg;
            reg["studentNo"] = "TESTREG001";
            reg["password"] = "another";

            StudentService::Register(db, reg.dump());
            FAIL("Expected duplicate register to throw");
        }
        catch (const std::exception &)
        {
            PASS(); // Expected to throw
        }

        // ====== Step 7c: Login with registered account ======
        TEST_STEP("Login with registered account");

        try
        {
            auto result = StudentService::Login(db, "TESTREG001", "regpass123");
            auto rj = nlohmann::json::parse(result);
            if (rj.contains("token") && rj["role"] == "student")
            {
                PASS();
            }
            else
            {
                FAIL("Login with registered account failed: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 8: Admin ListUsers ======
        TEST_STEP("Admin ListUsers");

        try
        {
            auto result = AdminService::ListUsers(db);
            auto rj = nlohmann::json::parse(result);
            if (rj.contains("users") && rj["users"].is_array() && rj["users"].size() >= 13)
            {
                std::cout << "PASS  (" << rj["users"].size() << " users)" << std::endl;
                ++g_pass;
            }
            else
            {
                FAIL("Expected >= 13 users: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 9: Admin DeleteUser ======
        TEST_STEP("Admin DeleteUser");

        if (!registered_user_id.empty())
        {
            try
            {
                auto result = AdminService::DeleteUser(db, "admin_001", registered_user_id);
                auto rj = nlohmann::json::parse(result);
                if (rj["deleted"] == true)
                {
                    PASS();
                }
                else
                {
                    FAIL("Delete failed: " + result);
                }
            }
            catch (const std::exception &e)
            {
                FAIL(e.what());
            }
        }
        else
        {
            FAIL("No registered user to delete");
        }

        // ====== Step 9b: Delete admin should fail ======
        TEST_STEP("Delete admin should fail");

        try
        {
            AdminService::DeleteUser(db, "admin_001", "admin_001");
            FAIL("Expected delete admin to throw");
        }
        catch (const std::exception &)
        {
            PASS(); // Expected to throw
        }

        // ====== Step 10: TransferAdmin ======
        TEST_STEP("TransferAdmin to TEST1001");

        std::string test1001_user_id;
        try
        {
            auto rs = db.ExecuteQuery("SELECT user_id FROM `user` WHERE student_no = 'TEST1001';");
            if (rs->next())
            {
                test1001_user_id = rs->getString("user_id").asStdString();
            }

            auto result = AdminService::TransferAdmin(db, "admin_001", test1001_user_id);
            auto rj = nlohmann::json::parse(result);
            if (rj["transferred"] == true)
            {
                PASS();
            }
            else
            {
                FAIL("Transfer failed: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 10b: Transfer back ======
        TEST_STEP("TransferAdmin back to admin");

        try
        {
            auto result = AdminService::TransferAdmin(db, test1001_user_id, "admin_001");
            auto rj = nlohmann::json::parse(result);
            if (rj["transferred"] == true)
            {
                PASS();
            }
            else
            {
                FAIL("Transfer back failed: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 11: Submit questionnaires ======
        TEST_STEP("Submit questionnaires (12 students)");

        const char *sleep_opts[] = {"early", "normal", "late"};
        const char *gaming_opts[] = {"never", "sometimes", "often"};
        const char *mbti_opts[] = {"INTJ", "ENFP", "ISTP", "ESTJ", "INFP", "ENTJ",
                                   "ISFJ", "ENTP", "INFJ", "ESFP", "ISTJ", "ENFJ"};

        int submitted = 0;
        for (int i = 0; i < 12; ++i)
        {
            std::string sno = "TEST" + std::to_string(i + 1001);
            auto rs = db.ExecuteQuery(
                "SELECT user_id FROM `user` WHERE student_no = '" + sno + "';");
            if (!rs->next())
            {
                std::cerr << "  -> User not found: " << sno << std::endl;
                continue;
            }
            std::string user_id = rs->getString("user_id").asStdString();

            nlohmann::json data;
            data["userId"] = user_id;
            data["basicInfo"]["dormType"] = 4;
            data["questionnaire"]["sleepSchedule"] = sleep_opts[i % 3];
            data["questionnaire"]["hygieneLevel"] = 2 + (i % 4);
            data["questionnaire"]["noiseTolerance"] = 2 + (i % 4);
            data["questionnaire"]["temperaturePreference"] = 22;
            data["questionnaire"]["socialPreference"] = 2 + (i % 4);
            data["questionnaire"]["gamingBehavior"] = gaming_opts[i % 3];
            data["questionnaire"]["mbtiType"] = mbti_opts[i];
            data["preference"]["similarityWeight"] = 0.5;
            data["preference"]["complementarityWeight"] = 0.2;
            data["preference"]["vetoSafetyWeight"] = 0.3;
            data["vetoItems"] = nlohmann::json::array({"late"});

            try
            {
                StudentService::SubmitQuestionnaire(db, user_id, data.dump());
                submitted++;
            }
            catch (const std::exception &e)
            {
                std::cerr << "  -> Submit failed for " << sno << ": " << e.what() << std::endl;
            }
        }

        if (submitted == 12)
        {
            PASS();
        }
        else
        {
            FAIL("Expected 12 submissions, got " + std::to_string(submitted));
        }

        // ====== Step 12: Add test dormitories ======
        TEST_STEP("Add test dormitories (6 rooms)");

        int dorm_count = 0;
        const char *dorm_buildings[] = {"A", "A", "B", "B", "C", "C"};
        const char *dorm_genders[] = {"male", "male", "female", "female", "male", "female"};
        const char *dorm_rooms[] = {"101", "102", "201", "202", "301", "302"};

        for (int i = 0; i < 6; ++i)
        {
            nlohmann::json dorm;
            dorm["building"] = dorm_buildings[i];
            dorm["roomNumber"] = dorm_rooms[i];
            dorm["capacity"] = 4;
            dorm["gender"] = dorm_genders[i];

            try
            {
                auto result = AdminService::AddDormitory(db, dorm.dump());
                auto rj = nlohmann::json::parse(result);
                if (rj.contains("dormId"))
                {
                    auto dorm_id = rj["dormId"].get<std::string>();
                    std::string test_dorm_id = "test_dorm_" + std::to_string(i);
                    db.Execute("UPDATE dormitory SET dorm_id = '" + test_dorm_id +
                               "' WHERE dorm_id = '" + dorm_id + "';");
                    dorm_count++;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "  -> Add dorm failed: " << e.what() << std::endl;
            }
        }

        if (dorm_count == 6)
        {
            PASS();
        }
        else
        {
            FAIL("Expected 6 dorms, got " + std::to_string(dorm_count));
        }

        // ====== Step 13: Create allocation task ======
        TEST_STEP("Create allocation task");

        std::string task_id;
        try
        {
            nlohmann::json task;
            task["taskName"] = "Test Allocation Task";
            task["college"] = "";
            task["major"] = "";
            task["gender"] = "";

            auto result = AdminService::CreateTask(db, task.dump());
            auto rj = nlohmann::json::parse(result);
            task_id = rj["taskId"].get<std::string>();

            std::string test_task_id = "test_task_main";
            db.Execute("UPDATE allocation_task SET task_id = '" + test_task_id +
                       "' WHERE task_id = '" + task_id + "';");
            task_id = test_task_id;

            PASS();
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 14: Run allocation ======
        TEST_STEP("Run allocation algorithm");

        int allocated = -1;
        if (!task_id.empty())
        {
            try
            {
                auto result = AdminService::RunTask(db, task_id);
                auto rj = nlohmann::json::parse(result);
                allocated = rj["allocatedCount"].get<int>();

                if (allocated > 0)
                {
                    PASS();
                }
                else
                {
                    FAIL("Allocated count is 0 or negative: " + result);
                }
            }
            catch (const std::exception &e)
            {
                FAIL(e.what());
            }
        }
        else
        {
            FAIL("No task_id available (previous step failed)");
        }

        // ====== Step 15: Verify match results ======
        TEST_STEP("Verify match results exist");

        try
        {
            auto rs = db.ExecuteQuery(
                "SELECT user_id FROM `user` WHERE student_no = 'TEST1001';");
            if (rs->next())
            {
                std::string uid = rs->getString("user_id").asStdString();
                auto result_json = StudentService::GetMatchResult(db, uid);
                auto rj = nlohmann::json::parse(result_json);
                if (rj.contains("resultId") && rj.contains("building"))
                {
                    std::cout << "PASS  (building: " << rj["building"].get<std::string>()
                              << ", room: " << rj["roomNumber"].get<std::string>() << ")" << std::endl;
                    ++g_pass;
                }
                else
                {
                    FAIL("No match result found for TEST1001: " + result_json);
                }
            }
            else
            {
                FAIL("TEST1001 user not found");
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 16: Manual adjust ======
        TEST_STEP("Manual adjust (swap students)");

        if (allocated > 1)
        {
            try
            {
                auto rs = db.ExecuteQuery(
                    "SELECT ANY_VALUE(mr.user_id) AS user_id, mr.dorm_id FROM match_result mr "
                    "JOIN `user` u ON mr.user_id = u.user_id "
                    "WHERE u.student_no LIKE 'TEST%' "
                    "GROUP BY mr.dorm_id ORDER BY mr.dorm_id LIMIT 2;");

                std::string user1, user2;
                if (rs->next())
                    user1 = rs->getString("user_id").asStdString();
                if (rs->next())
                    user2 = rs->getString("user_id").asStdString();

                if (!user1.empty() && !user2.empty())
                {
                    nlohmann::json adjust;
                    adjust["taskId"] = task_id;
                    adjust["userId1"] = user1;
                    adjust["userId2"] = user2;

                    auto result = AdminService::AdjustResult(db, adjust.dump());
                    auto rj = nlohmann::json::parse(result);
                    if (rj["swapped"] == true)
                    {
                        PASS();
                    }
                    else
                    {
                        FAIL("Swap not confirmed: " + result);
                    }
                }
                else
                {
                    FAIL("Could not find two students in different dorms");
                }
            }
            catch (const std::exception &e)
            {
                FAIL(e.what());
            }
        }
        else
        {
            FAIL("Not enough allocated students to test swap");
        }

        // ====== Step 17: Export CSV ======
        TEST_STEP("Export result as CSV");

        try
        {
            auto csv = AdminService::ExportResult(db, task_id);
            size_t lines = 0;
            for (char c : csv)
            {
                if (c == '\n')
                    lines++;
            }
            if (lines > 2 && csv.find("student_no") != std::string::npos)
            {
                std::cout << "PASS  (" << (lines - 1) << " data rows)" << std::endl;
                ++g_pass;
            }
            else
            {
                FAIL("CSV output too short or missing header");
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 18: List tasks ======
        TEST_STEP("List allocation tasks");

        try
        {
            auto result = AdminService::ListTasks(db);
            auto rj = nlohmann::json::parse(result);
            if (rj.contains("tasks") && rj["tasks"].is_array() && rj["tasks"].size() > 0)
            {
                PASS();
            }
            else
            {
                FAIL("No tasks listed: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 19: List dormitories ======
        TEST_STEP("List dormitories");

        try
        {
            auto result = AdminService::ListDormitories(db);
            auto rj = nlohmann::json::parse(result);
            if (rj.contains("dormitories") && rj["dormitories"].is_array() && rj["dormitories"].size() >= 6)
            {
                PASS();
            }
            else
            {
                FAIL("Expected >= 6 dormitories: " + result);
            }
        }
        catch (const std::exception &e)
        {
            FAIL(e.what());
        }

        // ====== Step 20: Cleanup test data ======
        TEST_STEP("Cleanup all test data");
        CleanupTestData(db);
        PASS();

        // ====== Summary ======
        std::cout << "\n=====================================" << std::endl;
        if (g_fail == 0)
        {
            std::cout << "  ALL " << g_pass << " TESTS PASSED" << std::endl;
        }
        else
        {
            std::cout << "  " << g_pass << " PASSED, " << g_fail << " FAILED" << std::endl;
        }
        std::cout << "=====================================" << std::endl;

        return g_fail > 0 ? 1 : 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[test] fatal: " << e.what() << std::endl;
        return 1;
    }
}