//此文件为宿舍分配系统的后端CLI工具，提供基本的命令行接口用于调试和验证数据库连接等功能。
//主要功能包括：
//- health: 打印系统的存活状态，输出 "OK"。
//- db ping: 尝试连接到MySQL数据库，并确保指定的数据库存在，如果成功则输出相关信息。



#include <iostream>

#include <CLI/CLI.hpp>

#include "infrastructure/config/AppConfig.h"
#include "infrastructure/db/DbBootstrapper.h"
#include "infrastructure/db/MySqlClient.h"

// Minimal CLI for backend debugging without frontend.
//
// Why CLI is useful:
// - When frontend is not ready, we can still verify DB connectivity and core logic.
// - It can be used in CI later to run batch tasks.
//
// Commands implemented in skeleton stage:
// - health                : prints OK
// - db ping               : tries to connect to MySQL and ensure database exists

static const char *kDefaultConfigPath = "config/config.example.json";

int main(int argc, char **argv)
{
    CLI::App app{"Dormitory Allocation CLI (debug tool)"};

    std::string config_path = kDefaultConfigPath;
    app.add_option("--config", config_path, "Path to config JSON");

    bool cmd_health = false;
    auto *health = app.add_subcommand("health", "Print liveness status");
    health->callback([&]()
                     { cmd_health = true; });

    bool cmd_db_ping = false;
    auto *db = app.add_subcommand("db", "Database operations");
    auto *ping = db->add_subcommand("ping", "Connect to MySQL and ensure database exists");
    ping->callback([&]()
                   { cmd_db_ping = true; });

    try
    {
        CLI11_PARSE(app, argc, argv);

        if (cmd_health)
        {
            std::cout << "OK" << std::endl;
            return 0;
        }

        if (cmd_db_ping)
        {
            const auto cfg = dorm_alloc::infra::config::LoadFromJsonFile(config_path);

            dorm_alloc::infra::db::MySqlClient client(cfg.mysql);
            client.ConnectServer();
            dorm_alloc::infra::db::DbBootstrapper::EnsureDatabaseExists(client, cfg.mysql.database);

            std::cout << "MySQL OK. Database ensured: " << cfg.mysql.database << std::endl;
            return 0;
        }

        // If no subcommand is given, show help.
        std::cout << app.help() << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[cli] error: " << e.what() << std::endl;
        return 1;
    }
}
