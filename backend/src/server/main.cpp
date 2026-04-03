//此文件为宿舍分配系统后端的主入口，负责启动HTTP服务器并确保MySQL数据库存在。
//主要功能包括：
//1) 加载配置文件：从指定路径加载JSON格式的配置，包含HTTP服务器和MySQL数据库的连接信息。
//2) MySQL数据库引导：使用MySqlClient连接到MySQL服务器，并调用DbBootstrapper确保指定的数据库存在，如果不存在则创建它。
//3) 启动HTTP服务器：使用httplib库创建一个简单的HTTP服务器

#include <cstdlib>
#include <iostream>

#include <httplib.h>

#include "infrastructure/config/AppConfig.h"
#include "infrastructure/db/DbBootstrapper.h"
#include "infrastructure/db/MySqlClient.h"

// This is the minimal HTTP server entry point.
//
// At skeleton stage we only provide:
// - /health   : quick liveness check
//
// The key non-functional requirement implemented here:
// - Ensure MySQL database exists on every startup.

static const char *kDefaultConfigPath = "config/config.example.json";

int main(int argc, char **argv)
{
    try
    {
        // --- 1) Load config ---
        // We keep it simple: accept an optional `--config` argument.
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

        // --- 2) MySQL bootstrap: ensure database exists ---
        dorm_alloc::infra::db::MySqlClient client(cfg.mysql);
        client.ConnectServer();
        dorm_alloc::infra::db::DbBootstrapper::EnsureDatabaseExists(client, cfg.mysql.database);

        // --- 3) Start HTTP server ---
        httplib::Server svr;

        svr.Get("/health", [](const httplib::Request & /*req*/, httplib::Response &res)
                { res.set_content("OK", "text/plain"); });

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
