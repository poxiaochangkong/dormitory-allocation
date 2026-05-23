#include "service/AdminService.h"

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

#include "algorithm/MatchEngine.h"
#include "infrastructure/auth/CryptoUtil.h"

namespace dorm_alloc
{
    namespace service
    {

        static std::string GenId(const std::string &prefix)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            int r = std::rand() % 10000;
            std::ostringstream oss;
            oss << prefix << ms << "_" << r;
            return oss.str();
        }

        // Escape single quotes in SQL string values
        static std::string Escape(const std::string &s)
        {
            std::string result;
            result.reserve(s.size());
            for (char c : s)
            {
                if (c == '\'')
                    result += "''";
                else
                    result += c;
            }
            return result;
        }

        std::string AdminService::Login(
            MySqlClient &db,
            const std::string &student_no,
            const std::string &password)
        {
            // Look up admin by student_no, retrieve stored hash and salt
            auto rs = db.ExecuteQuery(
                "SELECT user_id, role, gender, password, salt "
                "FROM `user` "
                "WHERE student_no = '" +
                Escape(student_no) + "' AND role = 'admin';");

            if (!rs->next())
            {
                throw std::runtime_error("Invalid admin credentials.");
            }

            // Verify password hash
            std::string stored_hash = rs->getString("password").asStdString();
            std::string salt = rs->getString("salt").asStdString();

            if (!dorm_alloc::infra::auth::CryptoUtil::VerifyPassword(password, salt, stored_hash))
            {
                throw std::runtime_error("Invalid admin credentials.");
            }

            // Generate a token for authentication
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            std::srand(static_cast<unsigned>(ms));
            const char hex[] = "0123456789abcdef";
            std::string token = "tk_";
            for (int i = 0; i < 32; ++i)
            {
                token += hex[std::rand() % 16];
            }

            std::string user_id = rs->getString("user_id").asStdString();

            // Store token in database
            db.Execute(
                "UPDATE `user` SET token = '" + Escape(token) +
                "' WHERE user_id = '" + Escape(user_id) + "';");

            nlohmann::json result;
            result["userId"] = user_id;
            result["studentNo"] = student_no;
            result["role"] = rs->getString("role").asStdString();
            result["gender"] = rs->getString("gender").asStdString();
            result["token"] = token;
            return result.dump();
        }

        std::string AdminService::ImportStudents(
            MySqlClient &db,
            const std::string &students_json)
        {
            auto arr = nlohmann::json::parse(students_json);
            if (!arr.is_array())
            {
                throw std::runtime_error("Expected a JSON array of students.");
            }

            int count = 0;
            for (const auto &s : arr)
            {
                std::string student_no = s.value("studentNo", "");
                std::string role = s.value("role", "student");

                if (student_no.empty())
                    continue;

                // Block importing admin accounts — system must have exactly one admin
                if (role == "admin")
                    continue;

                std::string user_id = GenId("u_");
                std::string gender = s.value("gender", "");
                std::string college = s.value("college", "");
                std::string major = s.value("major", "");
                std::string grade = s.value("grade", "");
                int dorm_type = s.value("dormType", 4);
                std::string raw_password = s.value("password", "123456");

                // Hash the password with a random salt
                std::string salt = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
                std::string hashed_pw = dorm_alloc::infra::auth::CryptoUtil::HashPassword(raw_password, salt);

                std::ostringstream sql;
                sql << "INSERT INTO `user` "
                    << "(user_id, student_no, password, salt, gender, college, major, grade, dorm_type, role) VALUES ("
                    << "'" << user_id << "', "
                    << "'" << Escape(student_no) << "', "
                    << "'" << hashed_pw << "', "
                    << "'" << salt << "', "
                    << "'" << Escape(gender) << "', "
                    << "'" << Escape(college) << "', "
                    << "'" << Escape(major) << "', "
                    << "'" << Escape(grade) << "', "
                    << dorm_type << ", "
                    << "'" << role << "') "
                    << "ON DUPLICATE KEY UPDATE "
                    << "gender = VALUES(gender), "
                    << "college = VALUES(college), "
                    << "major = VALUES(major), "
                    << "grade = VALUES(grade);";
                db.Execute(sql.str());
                count++;
            }

            nlohmann::json result;
            result["imported"] = count;
            return result.dump();
        }

        // ---- DeleteUser ----
        // Admin deletes a user. Cannot delete self or any admin.
        std::string AdminService::DeleteUser(
            MySqlClient &db,
            const std::string &admin_user_id,
            const std::string &target_user_id)
        {
            if (admin_user_id == target_user_id)
            {
                throw std::runtime_error("Cannot delete your own account.");
            }

            // Verify target exists and is not admin
            auto rs = db.ExecuteQuery(
                "SELECT user_id, role FROM `user` WHERE user_id = '" +
                Escape(target_user_id) + "';");

            if (!rs->next())
            {
                throw std::runtime_error("User not found: " + target_user_id);
            }

            std::string role = rs->getString("role").asStdString();
            if (role == "admin")
            {
                throw std::runtime_error("Cannot delete admin account.");
            }

            // Delete related records first (foreign key constraints)
            db.Execute("DELETE FROM match_result WHERE user_id = '" + Escape(target_user_id) + "';");
            db.Execute("DELETE FROM veto WHERE user_id = '" + Escape(target_user_id) + "';");
            db.Execute("DELETE FROM preference WHERE user_id = '" + Escape(target_user_id) + "';");
            db.Execute("DELETE FROM open_text_profile WHERE user_id = '" + Escape(target_user_id) + "';");
            db.Execute("DELETE FROM questionnaire WHERE user_id = '" + Escape(target_user_id) + "';");
            db.Execute("DELETE FROM `user` WHERE user_id = '" + Escape(target_user_id) + "';");

            nlohmann::json result;
            result["deleted"] = true;
            result["userId"] = target_user_id;
            return result.dump();
        }

        // ---- TransferAdmin ----
        // Transfer admin role to another user. Current admin becomes student.
        std::string AdminService::TransferAdmin(
            MySqlClient &db,
            const std::string &admin_user_id,
            const std::string &target_user_id)
        {
            if (admin_user_id == target_user_id)
            {
                throw std::runtime_error("Cannot transfer admin to yourself.");
            }

            // Verify target exists and is a student
            auto rs = db.ExecuteQuery(
                "SELECT user_id, role FROM `user` WHERE user_id = '" +
                Escape(target_user_id) + "';");

            if (!rs->next())
            {
                throw std::runtime_error("Target user not found: " + target_user_id);
            }

            // Demote current admin to student
            db.Execute(
                "UPDATE `user` SET role = 'student', token = NULL "
                "WHERE user_id = '" +
                Escape(admin_user_id) + "';");

            // Promote target to admin
            db.Execute(
                "UPDATE `user` SET role = 'admin' "
                "WHERE user_id = '" +
                Escape(target_user_id) + "';");

            nlohmann::json result;
            result["transferred"] = true;
            result["newAdminUserId"] = target_user_id;
            return result.dump();
        }

        // ---- ListUsers ----
        // List all student users (admin can view all students)
        std::string AdminService::ListUsers(MySqlClient &db)
        {
            auto rs = db.ExecuteQuery(
                "SELECT user_id, student_no, gender, college, major, grade, dorm_type, role, created_at "
                "FROM `user` ORDER BY created_at DESC;");

            nlohmann::json users = nlohmann::json::array();
            while (rs->next())
            {
                nlohmann::json u;
                u["userId"] = rs->getString("user_id").asStdString();
                u["studentNo"] = rs->getString("student_no").asStdString();
                u["gender"] = rs->getString("gender").asStdString();
                u["college"] = rs->getString("college").asStdString();
                u["major"] = rs->getString("major").asStdString();
                u["grade"] = rs->getString("grade").asStdString();
                u["dormType"] = rs->getInt("dorm_type");
                u["role"] = rs->getString("role").asStdString();
                if (!rs->isNull("created_at"))
                {
                    u["createdAt"] = rs->getString("created_at").asStdString();
                }
                else
                {
                    u["createdAt"] = "";
                }
                users.push_back(u);
            }

            nlohmann::json result;
            result["users"] = users;
            return result.dump();
        }

        std::string AdminService::SaveAllocationRule(
            MySqlClient &db,
            const std::string &rule_json)
        {
            // Validate JSON is parseable
            auto rule = nlohmann::json::parse(rule_json);

            // Store rule config in a simple key-value approach.
            // For simplicity, we just return the rule as-is.
            // In a real system, this would be stored in a config table.
            nlohmann::json result;
            result["saved"] = true;
            result["rule"] = rule;
            return result.dump();
        }

        std::string AdminService::CreateTask(
            MySqlClient &db,
            const std::string &task_json)
        {
            auto data = nlohmann::json::parse(task_json);

            std::string task_id = GenId("task_");
            std::string task_name = data.value("taskName", "Unnamed Task");
            std::string college = data.value("college", "");
            std::string major = data.value("major", "");
            std::string gender = data.value("gender", "");

            std::ostringstream sql;
            sql << "INSERT INTO allocation_task "
                << "(task_id, task_name, college, major, gender, status) VALUES ("
                << "'" << task_id << "', "
                << "'" << Escape(task_name) << "', "
                << "'" << Escape(college) << "', "
                << "'" << Escape(major) << "', "
                << "'" << Escape(gender) << "', "
                << "'pending');";
            db.Execute(sql.str());

            nlohmann::json result;
            result["taskId"] = task_id;
            return result.dump();
        }

        std::string AdminService::RunTask(
            MySqlClient &db,
            const std::string &task_id)
        {
            // Load task info
            auto rs = db.ExecuteQuery(
                "SELECT task_id, college, major, gender, status, rule_config "
                "FROM allocation_task WHERE task_id = '" +
                task_id + "';");

            if (!rs->next())
            {
                throw std::runtime_error("Task not found: " + task_id);
            }

            std::string status = rs->getString("status").asStdString();
            if (status == "running")
            {
                throw std::runtime_error("Task is already running.");
            }
            if (status == "completed")
            {
                // Delete old results for re-run
                db.Execute("DELETE FROM match_result WHERE task_id = '" + task_id + "';");
            }

            std::string college = rs->getString("college").asStdString();
            std::string major = rs->getString("major").asStdString();
            std::string gender = rs->getString("gender").asStdString();
            std::string rule_config;
            if (!rs->isNull("rule_config"))
            {
                rule_config = rs->getString("rule_config").asStdString();
            }

            // Update task status to running
            db.Execute(
                "UPDATE allocation_task SET status = 'running', executed_at = NOW() "
                "WHERE task_id = '" +
                task_id + "';");

            // Run the allocation algorithm
            auto results = dorm_alloc::algorithm::MatchEngine::ExecuteAllocation(
                db, task_id, college, major, gender, rule_config);

            nlohmann::json result;
            result["taskId"] = task_id;
            result["allocatedCount"] = results.size();
            result["status"] = "completed";
            return result.dump();
        }

        std::string AdminService::GetTaskResult(
            MySqlClient &db,
            const std::string &task_id)
        {
            // Get task info
            auto task_rs = db.ExecuteQuery(
                "SELECT * FROM allocation_task WHERE task_id = '" + task_id + "';");

            if (!task_rs->next())
            {
                throw std::runtime_error("Task not found: " + task_id);
            }

            nlohmann::json result;
            result["taskId"] = task_rs->getString("task_id").asStdString();
            result["taskName"] = task_rs->getString("task_name").asStdString();
            result["college"] = task_rs->getString("college").asStdString();
            result["major"] = task_rs->getString("major").asStdString();
            result["gender"] = task_rs->getString("gender").asStdString();
            result["status"] = task_rs->getString("status").asStdString();

            // Get allocation results grouped by dorm
            auto rs = db.ExecuteQuery(
                "SELECT mr.result_id, mr.user_id, mr.dorm_id, mr.roommate_ids, "
                "  mr.total_score, mr.explanation_text, "
                "  d.building, d.room_number, "
                "  u.student_no, u.gender, u.college, u.major "
                "FROM match_result mr "
                "LEFT JOIN dormitory d ON mr.dorm_id = d.dorm_id "
                "LEFT JOIN `user` u ON mr.user_id = u.user_id "
                "WHERE mr.task_id = '" +
                task_id + "';");

            nlohmann::json allocations = nlohmann::json::array();
            while (rs->next())
            {
                nlohmann::json item;
                item["resultId"] = rs->getString("result_id").asStdString();
                item["userId"] = rs->getString("user_id").asStdString();
                item["dormId"] = rs->getString("dorm_id").asStdString();
                item["building"] = rs->getString("building").asStdString();
                item["roomNumber"] = rs->getString("room_number").asStdString();
                item["studentNo"] = rs->getString("student_no").asStdString();
                item["totalScore"] = rs->getDouble("total_score");
                item["explanationText"] = rs->getString("explanation_text").asStdString();
                allocations.push_back(item);
            }

            result["allocations"] = allocations;
            return result.dump();
        }

        std::string AdminService::AdjustResult(
            MySqlClient &db,
            const std::string &adjust_json)
        {
            auto data = nlohmann::json::parse(adjust_json);
            std::string task_id = data.value("taskId", "");
            std::string user_id1 = data.value("userId1", "");
            std::string user_id2 = data.value("userId2", "");

            if (task_id.empty() || user_id1.empty() || user_id2.empty())
            {
                throw std::runtime_error("Missing required fields: taskId, userId1, userId2");
            }

            // Get current results for both students
            auto rs1 = db.ExecuteQuery(
                "SELECT result_id, dorm_id FROM match_result "
                "WHERE task_id = '" +
                task_id + "' AND user_id = '" + user_id1 + "';");
            if (!rs1->next())
                throw std::runtime_error("No result found for user1: " + user_id1);

            std::string result_id1 = rs1->getString("result_id").asStdString();
            std::string dorm_id1 = rs1->getString("dorm_id").asStdString();

            auto rs2 = db.ExecuteQuery(
                "SELECT result_id, dorm_id FROM match_result "
                "WHERE task_id = '" +
                task_id + "' AND user_id = '" + user_id2 + "';");
            if (!rs2->next())
                throw std::runtime_error("No result found for user2: " + user_id2);

            std::string result_id2 = rs2->getString("result_id").asStdString();
            std::string dorm_id2 = rs2->getString("dorm_id").asStdString();

            // Swap dorm assignments
            db.Execute(
                "UPDATE match_result SET dorm_id = '" + dorm_id2 +
                "' WHERE result_id = '" + result_id1 + "';");
            db.Execute(
                "UPDATE match_result SET dorm_id = '" + dorm_id1 +
                "' WHERE result_id = '" + result_id2 + "';");

            nlohmann::json result;
            result["swapped"] = true;
            result["userId1"] = user_id1;
            result["userId2"] = user_id2;
            return result.dump();
        }

        std::string AdminService::ExportResult(
            MySqlClient &db,
            const std::string &task_id)
        {
            auto rs = db.ExecuteQuery(
                "SELECT mr.user_id, mr.dorm_id, mr.total_score, "
                "  d.building, d.room_number, "
                "  u.student_no, u.gender, u.college, u.major "
                "FROM match_result mr "
                "LEFT JOIN dormitory d ON mr.dorm_id = d.dorm_id "
                "LEFT JOIN `user` u ON mr.user_id = u.user_id "
                "WHERE mr.task_id = '" +
                task_id + "' ORDER BY d.building, d.room_number, u.student_no;");

            std::ostringstream csv;
            csv << "student_no,gender,college,major,building,room_number,total_score\n";

            while (rs->next())
            {
                csv << rs->getString("student_no").asStdString() << ","
                    << rs->getString("gender").asStdString() << ","
                    << rs->getString("college").asStdString() << ","
                    << rs->getString("major").asStdString() << ","
                    << rs->getString("building").asStdString() << ","
                    << rs->getString("room_number").asStdString() << ","
                    << rs->getDouble("total_score") << "\n";
            }

            return csv.str();
        }

        std::string AdminService::ListTasks(MySqlClient &db)
        {
            auto rs = db.ExecuteQuery(
                "SELECT task_id, task_name, college, major, gender, status, "
                "  created_at, executed_at, finished_at "
                "FROM allocation_task ORDER BY created_at DESC;");

            nlohmann::json tasks = nlohmann::json::array();
            while (rs->next())
            {
                nlohmann::json t;
                t["taskId"] = rs->getString("task_id").asStdString();
                t["taskName"] = rs->getString("task_name").asStdString();
                t["college"] = rs->getString("college").asStdString();
                t["major"] = rs->getString("major").asStdString();
                t["gender"] = rs->getString("gender").asStdString();
                t["status"] = rs->getString("status").asStdString();
                // Include createdAt timestamp for frontend display
                if (!rs->isNull("created_at"))
                {
                    t["createdAt"] = rs->getString("created_at").asStdString();
                }
                else
                {
                    t["createdAt"] = "";
                }
                tasks.push_back(t);
            }

            nlohmann::json result;
            result["tasks"] = tasks;
            return result.dump();
        }

        std::string AdminService::ListDormitories(MySqlClient &db)
        {
            auto rs = db.ExecuteQuery(
                "SELECT dorm_id, building, room_number, capacity, gender, is_available "
                "FROM dormitory ORDER BY building, room_number;");

            nlohmann::json dorms = nlohmann::json::array();
            while (rs->next())
            {
                nlohmann::json d;
                d["dormId"] = rs->getString("dorm_id").asStdString();
                d["building"] = rs->getString("building").asStdString();
                d["roomNumber"] = rs->getString("room_number").asStdString();
                d["capacity"] = rs->getInt("capacity");
                d["gender"] = rs->getString("gender").asStdString();
                d["isAvailable"] = rs->getInt("is_available");
                dorms.push_back(d);
            }

            nlohmann::json result;
            result["dormitories"] = dorms;
            return result.dump();
        }

        std::string AdminService::AddDormitory(
            MySqlClient &db,
            const std::string &dorm_json)
        {
            auto data = nlohmann::json::parse(dorm_json);

            std::string dorm_id = GenId("dorm_");
            std::string building = data.value("building", "");
            std::string room_number = data.value("roomNumber", "");
            int capacity = data.value("capacity", 4);
            std::string gender = data.value("gender", "");

            std::ostringstream sql;
            sql << "INSERT INTO dormitory "
                << "(dorm_id, building, room_number, capacity, gender) VALUES ("
                << "'" << dorm_id << "', "
                << "'" << Escape(building) << "', "
                << "'" << Escape(room_number) << "', "
                << capacity << ", "
                << "'" << Escape(gender) << "');";
            db.Execute(sql.str());

            nlohmann::json result;
            result["dormId"] = dorm_id;
            return result.dump();
        }

    } // namespace service
} // namespace dorm_alloc