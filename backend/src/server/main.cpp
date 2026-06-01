// Main entry point for the dormitory allocation backend.
// Starts HTTP server with all API routes and ensures MySQL database/tables exist.

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>

#include <httplib.h>
#include <mysql/jdbc.h>
#include <nlohmann/json.hpp>

#include "infrastructure/config/AppConfig.h"
#include "infrastructure/db/DbBootstrapper.h"
#include "infrastructure/db/MySqlClient.h"
#include "infrastructure/log/Logger.h"
#include "service/StudentService.h"
#include "service/AdminService.h"

using MySqlClient = dorm_alloc::infra::db::MySqlClient;

static const char *kDefaultConfigPath = "config/config.example.json";

// Helper: build a success JSON response
static void JsonSuccess(httplib::Response &res, const std::string &data_json)
{
    nlohmann::json resp;
    resp["code"] = 200;
    resp["message"] = "success";
    resp["data"] = nlohmann::json::parse(data_json);
    res.set_content(resp.dump(), "application/json");
}

// Helper: build an error JSON response
static void JsonError(httplib::Response &res, int code, const std::string &message)
{
    nlohmann::json resp;
    resp["code"] = code;
    resp["message"] = message;
    resp["data"] = nullptr;
    res.set_content(resp.dump(), "application/json");
}

// Helper: set CORS headers on response
static void SetCors(httplib::Response &res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// Helper: authenticate request via Bearer token.
// Returns {userId, role} on success, or empty optional on failure.
struct AuthInfo
{
    std::string userId;
    std::string role;
};

static bool AuthenticateRequest(MySqlClient &db, const httplib::Request &req, AuthInfo &out)
{
    // Look for Authorization: Bearer <token>
    std::string auth_header = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth_header.size() <= prefix.size() ||
        auth_header.substr(0, prefix.size()) != prefix)
    {
        return false;
    }
    std::string token = auth_header.substr(prefix.size());

    // Query database for token
    try
    {
        std::ostringstream sql;
        sql << "SELECT user_id, role FROM `user` WHERE token = '" << token << "';";
        auto rs = db.ExecuteQuery(sql.str());
        if (rs->next())
        {
            out.userId = rs->getString("user_id").asStdString();
            out.role = rs->getString("role").asStdString();
            return true;
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("AuthenticateRequest DB error: {}", e.what());
    }
    return false;
}

int main(int argc, char **argv)
{
    try
    {
        // --- 1) Load config ---
        std::string config_path = kDefaultConfigPath;
        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--config" && i + 1 < argc)
            {
                config_path = argv[i + 1];
                ++i;
            }
        }

        const auto cfg = dorm_alloc::infra::config::LoadFromJsonFile(config_path);

        // --- 1.5) Initialize logger ---
        dorm_alloc::infra::log::InitLogger(cfg.logging.log_dir, cfg.logging.level);
        LOG_INFO("Config loaded from: {}", config_path);

        // --- 2) MySQL bootstrap ---
        MySqlClient client(cfg.mysql);
        client.ConnectServer();
        LOG_INFO("Connected to MySQL server at {}:{}", cfg.mysql.host, cfg.mysql.port);

        dorm_alloc::infra::db::DbBootstrapper::EnsureDatabaseExists(client, cfg.mysql.database);
        dorm_alloc::infra::db::DbBootstrapper::EnsureTablesExist(client);

        LOG_INFO("Database ready: {}", cfg.mysql.database);

        // --- 3) Start HTTP server ---
        httplib::Server svr;
        std::mutex db_mutex;

        // Global CORS preflight handler
        svr.Options(".*", [](const httplib::Request &, httplib::Response &res)
                    { SetCors(res); });

        // Health check
        svr.Get("/health", [](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    res.set_content("OK", "text/plain"); });

        // ===================== Student APIs =====================

        // POST /api/student/login
        svr.Post("/api/student/login", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string student_no = body.value("studentNo", "");
                         std::string password = body.value("password", "");
                         LOG_INFO("Student login attempt: studentNo={}", student_no);
                         auto result = dorm_alloc::service::StudentService::Login(client, student_no, password);
                         LOG_INFO("Student login success: studentNo={}", student_no);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_WARN("Student login failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/questionnaire/template
        svr.Get("/api/student/questionnaire/template", [&client](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        LOG_INFO("Fetching questionnaire template");
                        auto result = dorm_alloc::service::StudentService::GetQuestionnaireTemplate();
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Failed to get questionnaire template: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/student/questionnaire/submit
        svr.Post("/api/student/questionnaire/submit", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         // Auth check: verify token and match userId
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth))
                         {
                             LOG_WARN("Questionnaire submit rejected: unauthorized");
                             JsonError(res, 401, "Unauthorized: invalid or missing token.");
                             return;
                         }

                         auto body = nlohmann::json::parse(req.body);
                         std::string user_id = body.value("userId", "");
                         if (user_id.empty())
                         {
                             JsonError(res, 400, "Missing userId");
                             return;
                         }
                         // Verify the authenticated user matches the requested userId
                         if (auth.userId != user_id)
                         {
                             LOG_WARN("Questionnaire submit rejected: auth userId={} does not match body userId={}", auth.userId, user_id);
                             JsonError(res, 403, "Forbidden: userId mismatch.");
                             return;
                         }
                         LOG_INFO("Questionnaire submit: userId={}", user_id);
                         auto result = dorm_alloc::service::StudentService::SubmitQuestionnaire(client, user_id, req.body);
                         LOG_INFO("Questionnaire submitted successfully: userId={}", user_id);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Questionnaire submit failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/info/:userId
        svr.Get(R"(/api/student/info/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth))
                        {
                            LOG_WARN("Get student info rejected: unauthorized");
                            JsonError(res, 401, "Unauthorized: invalid or missing token.");
                            return;
                        }
                        std::string user_id = req.matches[1];
                        if (auth.userId != user_id)
                        {
                            LOG_WARN("Get student info rejected: auth userId={} does not match path userId={}", auth.userId, user_id);
                            JsonError(res, 403, "Forbidden: userId mismatch.");
                            return;
                        }
                        LOG_INFO("Get student info: userId={}", user_id);
                        auto result = dorm_alloc::service::StudentService::GetStudentInfo(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Get student info failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // GET /api/student/questionnaire/status/:userId
        svr.Get(R"(/api/student/questionnaire/status/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth))
                        {
                            LOG_WARN("Questionnaire status rejected: unauthorized");
                            JsonError(res, 401, "Unauthorized: invalid or missing token.");
                            return;
                        }
                        std::string user_id = req.matches[1];
                        if (auth.userId != user_id)
                        {
                            LOG_WARN("Questionnaire status rejected: auth userId={} does not match path userId={}", auth.userId, user_id);
                            JsonError(res, 403, "Forbidden: userId mismatch.");
                            return;
                        }
                        auto result = dorm_alloc::service::StudentService::GetQuestionnaireStatus(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Get questionnaire status failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/student/scene/submit
        svr.Post("/api/student/scene/submit", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth))
                         {
                             LOG_WARN("Scene submit rejected: unauthorized");
                             JsonError(res, 401, "Unauthorized: invalid or missing token.");
                             return;
                         }
                         auto body = nlohmann::json::parse(req.body);
                         std::string user_id = body.value("userId", "");
                         if (user_id.empty())
                         {
                             JsonError(res, 400, "Missing userId");
                             return;
                         }
                         if (auth.userId != user_id)
                         {
                             LOG_WARN("Scene submit rejected: auth userId={} does not match body userId={}", auth.userId, user_id);
                             JsonError(res, 403, "Forbidden: userId mismatch.");
                             return;
                         }
                         LOG_INFO("Scene data submit: userId={}", user_id);
                         auto result = dorm_alloc::service::StudentService::SubmitSceneData(client, user_id, req.body);
                         LOG_INFO("Scene data submitted successfully: userId={}", user_id);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Scene submit failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/student/register
        svr.Post("/api/student/register", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         LOG_INFO("Student register attempt");
                         auto result = dorm_alloc::service::StudentService::Register(client, req.body);
                         LOG_INFO("Student registered successfully");
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_WARN("Student register failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/match-result/:userId
        svr.Get(R"(/api/student/match-result/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth))
                        {
                            LOG_WARN("Get match result rejected: unauthorized");
                            JsonError(res, 401, "Unauthorized: invalid or missing token.");
                            return;
                        }
                        std::string user_id = req.matches[1];
                        if (auth.userId != user_id)
                        {
                            LOG_WARN("Get match result rejected: auth userId={} does not match path userId={}", auth.userId, user_id);
                            JsonError(res, 403, "Forbidden: userId mismatch.");
                            return;
                        }
                        LOG_INFO("Get match result: userId={}", user_id);
                        auto result = dorm_alloc::service::StudentService::GetMatchResult(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Get match result failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // ===================== Admin APIs =====================

        // POST /api/admin/login
        svr.Post("/api/admin/login", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string student_no = body.value("studentNo", "");
                         LOG_INFO("Admin login attempt: studentNo={}", student_no);
                         std::string password = body.value("password", "");
                         auto result = dorm_alloc::service::AdminService::Login(client, student_no, password);
                         LOG_INFO("Admin login success: studentNo={}", student_no);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_WARN("Admin login failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/students/import
        svr.Post("/api/admin/students/import", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Import students rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         LOG_INFO("Import students: requested by admin userId={}", auth.userId);
                         auto result = dorm_alloc::service::AdminService::ImportStudents(client, req.body);
                         LOG_INFO("Import students completed: {}", result);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Import students failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/rule/save
        svr.Post("/api/admin/allocation/rule/save", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Save allocation rule rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         LOG_INFO("Save allocation rule: requested by admin userId={}", auth.userId);
                         auto result = dorm_alloc::service::AdminService::SaveAllocationRule(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Save allocation rule failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/task/create
        svr.Post("/api/admin/allocation/task/create", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Create task rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         LOG_INFO("Create allocation task: requested by admin userId={}", auth.userId);
                         auto result = dorm_alloc::service::AdminService::CreateTask(client, req.body);
                         LOG_INFO("Task created: {}", result);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Create task failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/task/run/:taskId
        svr.Post(R"(/api/admin/allocation/task/run/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Run task rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         std::string task_id = req.matches[1];
                         LOG_INFO("Run allocation task: taskId={}, requested by admin userId={}", task_id, auth.userId);
                         auto result = dorm_alloc::service::AdminService::RunTask(client, task_id);
                         LOG_INFO("Task completed: {}", result);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Run task failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/allocation/task/result/:taskId
        svr.Get(R"(/api/admin/allocation/task/result/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                        {
                            LOG_WARN("Get task result rejected: unauthorized or not admin");
                            JsonError(res, 401, "Unauthorized: admin access required.");
                            return;
                        }
                        std::string task_id = req.matches[1];
                        LOG_INFO("Get task result: taskId={}", task_id);
                        auto result = dorm_alloc::service::AdminService::GetTaskResult(client, task_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Get task result failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/admin/allocation/task/adjust
        svr.Post("/api/admin/allocation/task/adjust", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Adjust result rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         LOG_INFO("Adjust allocation result: requested by admin userId={}", auth.userId);
                         auto result = dorm_alloc::service::AdminService::AdjustResult(client, req.body);
                         LOG_INFO("Adjust result completed: {}", result);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Adjust result failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/allocation/task/export/:taskId
        svr.Get(R"(/api/admin/allocation/task/export/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                        {
                            LOG_WARN("Export task result rejected: unauthorized or not admin");
                            JsonError(res, 401, "Unauthorized: admin access required.");
                            return;
                        }
                        std::string task_id = req.matches[1];
                        LOG_INFO("Export task result: taskId={}", task_id);
                        auto csv = dorm_alloc::service::AdminService::ExportResult(client, task_id);
                        res.set_content(csv, "text/csv");
                        res.set_header("Content-Disposition",
                                       "attachment; filename=\"allocation_result_" + task_id + ".csv\"");
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Export task result failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // DELETE /api/admin/allocation/task/:taskId
        svr.Delete(R"(/api/admin/allocation/task/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                   {
                       SetCors(res);
                       std::lock_guard<std::mutex> lock(db_mutex);
                       try
                       {
                           AuthInfo auth;
                           if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                           {
                               LOG_WARN("Delete task rejected: unauthorized or not admin");
                               JsonError(res, 401, "Unauthorized: admin access required.");
                               return;
                           }
                           std::string task_id = req.matches[1];
                           LOG_INFO("Delete task: taskId={}, requested by admin userId={}", task_id, auth.userId);
                           auto result = dorm_alloc::service::AdminService::DeleteTask(client, task_id);
                           LOG_INFO("Task deleted: taskId={}", task_id);
                           JsonSuccess(res, result);
                       }
                       catch (const std::exception &e)
                       {
                           LOG_ERROR("Delete task failed: {}", e.what());
                           JsonError(res, 400, e.what());
                       } });

        // GET /api/admin/tasks
        svr.Get("/api/admin/tasks", [&client, &db_mutex](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        // Note: No auth here to match existing frontend behavior
                        LOG_INFO("List all tasks");
                        auto result = dorm_alloc::service::AdminService::ListTasks(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("List tasks failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // GET /api/admin/dormitories
        svr.Get("/api/admin/dormitories", [&client, &db_mutex](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        LOG_INFO("List all dormitories");
                        auto result = dorm_alloc::service::AdminService::ListDormitories(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("List dormitories failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/admin/dormitories
        svr.Post("/api/admin/dormitories", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Add dormitory rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         LOG_INFO("Add dormitory: requested by admin userId={}", auth.userId);
                         auto result = dorm_alloc::service::AdminService::AddDormitory(client, req.body);
                         LOG_INFO("Dormitory added: {}", result);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Add dormitory failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // DELETE /api/admin/users/:userId
        svr.Delete(R"(/api/admin/users/([^/]+))", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                   {
                       SetCors(res);
                       std::lock_guard<std::mutex> lock(db_mutex);
                       try
                       {
                           AuthInfo auth;
                           if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                           {
                               LOG_WARN("Delete user rejected: unauthorized or not admin");
                               JsonError(res, 401, "Unauthorized: admin access required.");
                               return;
                           }
                           std::string target_id = req.matches[1];
                           LOG_INFO("Delete user: targetUserId={}, requested by admin userId={}", target_id, auth.userId);
                           auto result = dorm_alloc::service::AdminService::DeleteUser(client, auth.userId, target_id);
                           LOG_INFO("User deleted: userId={}", target_id);
                           JsonSuccess(res, result);
                       }
                       catch (const std::exception &e)
                       {
                           LOG_ERROR("Delete user failed: {}", e.what());
                           JsonError(res, 400, e.what());
                       } });

        // POST /api/admin/transfer
        svr.Post("/api/admin/transfer", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     std::lock_guard<std::mutex> lock(db_mutex);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
                             LOG_WARN("Transfer admin rejected: unauthorized or not admin");
                             JsonError(res, 401, "Unauthorized: admin access required.");
                             return;
                         }
                         auto body = nlohmann::json::parse(req.body);
                         std::string target_id = body.value("targetUserId", "");
                         if (target_id.empty())
                         {
                             JsonError(res, 400, "Missing targetUserId");
                             return;
                         }
                         LOG_INFO("Transfer admin: from userId={} to userId={}", auth.userId, target_id);
                         auto result = dorm_alloc::service::AdminService::TransferAdmin(client, auth.userId, target_id);
                         LOG_INFO("Admin transferred to userId={}", target_id);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         LOG_ERROR("Transfer admin failed: {}", e.what());
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/users
        svr.Get("/api/admin/users", [&client, &db_mutex](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    std::lock_guard<std::mutex> lock(db_mutex);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                        {
                            LOG_WARN("List users rejected: unauthorized or not admin");
                            JsonError(res, 401, "Unauthorized: admin access required.");
                            return;
                        }
                        LOG_INFO("List users: requested by admin userId={}", auth.userId);
                        auto result = dorm_alloc::service::AdminService::ListUsers(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("List users failed: {}", e.what());
                        JsonError(res, 400, e.what());
                    } });

        LOG_INFO("Server listening on {}:{}", cfg.http.host, cfg.http.port);
        std::cout << "[server] listening on " << cfg.http.host << ":" << cfg.http.port << std::endl;
        svr.listen(cfg.http.host.c_str(), cfg.http.port);

        LOG_INFO("Server stopped.");
        dorm_alloc::infra::log::ShutdownLogger();
        return 0;
    }
    catch (const std::exception &e)
    {
        LOG_CRITICAL("Server fatal error: {}", e.what());
        std::cerr << "[server] fatal: " << e.what() << std::endl;
        dorm_alloc::infra::log::ShutdownLogger();
        return 1;
    }
}