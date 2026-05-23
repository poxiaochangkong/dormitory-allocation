#pragma once

// Admin-facing business logic.
// Handles student import, task management, result queries, adjustments, and export.

#include <string>

#include "infrastructure/db/MySqlClient.h"

namespace dorm_alloc
{
    namespace service
    {

        using MySqlClient = dorm_alloc::infra::db::MySqlClient;

        class AdminService
        {
        public:
            // Import students from a JSON array.
            // Each item: { "studentNo":"...", "gender":"...", "college":"...", "major":"...", "grade":"..." }
            // Returns count of imported students.
            static std::string ImportStudents(MySqlClient &db,
                                              const std::string &students_json);

            // Save allocation rules (stored as JSON config).
            static std::string SaveAllocationRule(MySqlClient &db,
                                                  const std::string &rule_json);

            // Create a new allocation task.
            // Input JSON: { "taskName":"...", "college":"...", "major":"...", "gender":"..." }
            static std::string CreateTask(MySqlClient &db,
                                          const std::string &task_json);

            // Execute an allocation task by ID.
            static std::string RunTask(MySqlClient &db,
                                       const std::string &task_id);

            // Get task result summary.
            static std::string GetTaskResult(MySqlClient &db,
                                             const std::string &task_id);

            // Manual adjustment: swap two students' dorm assignments.
            // Input JSON: { "taskId":"...", "userId1":"...", "userId2":"..." }
            static std::string AdjustResult(MySqlClient &db,
                                            const std::string &adjust_json);

            // Export task result as CSV string.
            static std::string ExportResult(MySqlClient &db,
                                            const std::string &task_id);

            // Admin login (same as student but checks role = 'admin').
            static std::string Login(MySqlClient &db,
                                     const std::string &student_no,
                                     const std::string &password);

            // Get list of all allocation tasks.
            static std::string ListTasks(MySqlClient &db);

            // Get list of all dormitories.
            static std::string ListDormitories(MySqlClient &db);

            // Add a dormitory.
            static std::string AddDormitory(MySqlClient &db,
                                            const std::string &dorm_json);

            // Delete a user. Cannot delete self or any admin.
            static std::string DeleteUser(MySqlClient &db,
                                          const std::string &admin_user_id,
                                          const std::string &target_user_id);

            // Transfer admin role to another user.
            static std::string TransferAdmin(MySqlClient &db,
                                             const std::string &admin_user_id,
                                             const std::string &target_user_id);

            // List all users.
            static std::string ListUsers(MySqlClient &db);
        };

    } // namespace service
} // namespace dorm_alloc