// Main entry point for the dormitory allocation backend.
// Starts HTTP server with all API routes and ensures MySQL database/tables exist.

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <httplib.h>
#include <mysql/jdbc.h>
#include <nlohmann/json.hpp>

#include "infrastructure/config/AppConfig.h"
#include "infrastructure/db/DbBootstrapper.h"
#include "infrastructure/db/MySqlClient.h"
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
    catch (const std::exception &)
    {
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

        // --- 2) MySQL bootstrap ---
        MySqlClient client(cfg.mysql);
        client.ConnectServer();
        dorm_alloc::infra::db::DbBootstrapper::EnsureDatabaseExists(client, cfg.mysql.database);
        dorm_alloc::infra::db::DbBootstrapper::EnsureTablesExist(client);

        std::cout << "[server] database ready: " << cfg.mysql.database << std::endl;

        // --- 3) Start HTTP server ---
        httplib::Server svr;

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
        svr.Post("/api/student/login", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string student_no = body.value("studentNo", "");
                         std::string password = body.value("password", "");
                         auto result = dorm_alloc::service::StudentService::Login(client, student_no, password);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/questionnaire/template
        svr.Get("/api/student/questionnaire/template", [&client](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        auto result = dorm_alloc::service::StudentService::GetQuestionnaireTemplate();
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/student/questionnaire/submit
        svr.Post("/api/student/questionnaire/submit", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string user_id = body.value("userId", "");
                         if (user_id.empty())
                         {
                             JsonError(res, 400, "Missing userId");
                             return;
                         }
                         auto result = dorm_alloc::service::StudentService::SubmitQuestionnaire(client, user_id, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/info/:userId — get student basic info
        svr.Get(R"(/api/student/info/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        std::string user_id = req.matches[1];
                        auto result = dorm_alloc::service::StudentService::GetStudentInfo(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // GET /api/student/questionnaire/status/:userId — check questionnaire completion
        svr.Get(R"(/api/student/questionnaire/status/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        std::string user_id = req.matches[1];
                        auto result = dorm_alloc::service::StudentService::GetQuestionnaireStatus(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/student/scene/submit — submit immersive scene data
        svr.Post("/api/student/scene/submit", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string user_id = body.value("userId", "");
                         if (user_id.empty())
                         {
                             JsonError(res, 400, "Missing userId");
                             return;
                         }
                         auto result = dorm_alloc::service::StudentService::SubmitSceneData(client, user_id, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/student/register — student self-registration (no auth required)
        svr.Post("/api/student/register", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::StudentService::Register(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/student/match-result/:userId — path param to match frontend api/index.js
        svr.Get(R"(/api/student/match-result/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        std::string user_id = req.matches[1];
                        auto result = dorm_alloc::service::StudentService::GetMatchResult(client, user_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // ===================== Admin APIs =====================

        // POST /api/admin/login
        svr.Post("/api/admin/login", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto body = nlohmann::json::parse(req.body);
                         std::string student_no = body.value("studentNo", "");
                         std::string password = body.value("password", "");
                         auto result = dorm_alloc::service::AdminService::Login(client, student_no, password);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/students/import
        svr.Post("/api/admin/students/import", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::AdminService::ImportStudents(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/rule/save
        svr.Post("/api/admin/allocation/rule/save", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::AdminService::SaveAllocationRule(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/task/create
        svr.Post("/api/admin/allocation/task/create", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::AdminService::CreateTask(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // POST /api/admin/allocation/task/run/:taskId
        svr.Post(R"(/api/admin/allocation/task/run/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         std::string task_id = req.matches[1];
                         auto result = dorm_alloc::service::AdminService::RunTask(client, task_id);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/allocation/task/result/:taskId
        svr.Get(R"(/api/admin/allocation/task/result/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        std::string task_id = req.matches[1];
                        auto result = dorm_alloc::service::AdminService::GetTaskResult(client, task_id);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/admin/allocation/task/adjust
        svr.Post("/api/admin/allocation/task/adjust", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::AdminService::AdjustResult(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/allocation/task/export/:taskId
        svr.Get(R"(/api/admin/allocation/task/export/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        std::string task_id = req.matches[1];
                        auto csv = dorm_alloc::service::AdminService::ExportResult(client, task_id);
                        res.set_content(csv, "text/csv");
                        res.set_header("Content-Disposition",
                                       "attachment; filename=\"allocation_result_" + task_id + ".csv\"");
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // DELETE /api/admin/allocation/task/:taskId — delete a task and its results (requires auth)
        svr.Delete(R"(/api/admin/allocation/task/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                   {
                       SetCors(res);
                       try
                       {
                           AuthInfo auth;
                           if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                           {
                               JsonError(res, 401, "Unauthorized: admin access required.");
                               return;
                           }
                           std::string task_id = req.matches[1];
                           auto result = dorm_alloc::service::AdminService::DeleteTask(client, task_id);
                           JsonSuccess(res, result);
                       }
                       catch (const std::exception &e)
                       {
                           JsonError(res, 400, e.what());
                       } });

        // GET /api/admin/tasks - list all tasks
        svr.Get("/api/admin/tasks", [&client](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        auto result = dorm_alloc::service::AdminService::ListTasks(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // GET /api/admin/dormitories - list all dormitories
        svr.Get("/api/admin/dormitories", [&client](const httplib::Request &, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        auto result = dorm_alloc::service::AdminService::ListDormitories(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        // POST /api/admin/dormitories - add a dormitory
        svr.Post("/api/admin/dormitories", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         auto result = dorm_alloc::service::AdminService::AddDormitory(client, req.body);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // DELETE /api/admin/users/:userId — admin deletes a user (requires auth)
        svr.Delete(R"(/api/admin/users/([^/]+))", [&client](const httplib::Request &req, httplib::Response &res)
                   {
                       SetCors(res);
                       try
                       {
                           AuthInfo auth;
                           if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                           {
                               JsonError(res, 401, "Unauthorized: admin access required.");
                               return;
                           }
                           std::string target_id = req.matches[1];
                           auto result = dorm_alloc::service::AdminService::DeleteUser(client, auth.userId, target_id);
                           JsonSuccess(res, result);
                       }
                       catch (const std::exception &e)
                       {
                           JsonError(res, 400, e.what());
                       } });

        // POST /api/admin/transfer — transfer admin role (requires auth)
        svr.Post("/api/admin/transfer", [&client](const httplib::Request &req, httplib::Response &res)
                 {
                     SetCors(res);
                     try
                     {
                         AuthInfo auth;
                         if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                         {
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
                         auto result = dorm_alloc::service::AdminService::TransferAdmin(client, auth.userId, target_id);
                         JsonSuccess(res, result);
                     }
                     catch (const std::exception &e)
                     {
                         JsonError(res, 400, e.what());
                     } });

        // GET /api/admin/users — list all users (requires auth)
        svr.Get("/api/admin/users", [&client](const httplib::Request &req, httplib::Response &res)
                {
                    SetCors(res);
                    try
                    {
                        AuthInfo auth;
                        if (!AuthenticateRequest(client, req, auth) || auth.role != "admin")
                        {
                            JsonError(res, 401, "Unauthorized: admin access required.");
                            return;
                        }
                        auto result = dorm_alloc::service::AdminService::ListUsers(client);
                        JsonSuccess(res, result);
                    }
                    catch (const std::exception &e)
                    {
                        JsonError(res, 400, e.what());
                    } });

        std::cout << "[server] listening on " << cfg.http.host << ":" << cfg.http.port << std::endl;
        svr.listen(cfg.http.host.c_str(), cfg.http.port);
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[server] fatal: " << e.what() << std::endl;
        return 1;
    }
}