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
