#pragma once

// Database bootstrapper.
//
// Requirement from user:
// - "库不存在，需要每次启动程序时都验证库是否存在"
//
// Strategy:
// 1) Connect to server (no DB selected).
// 2) CREATE DATABASE IF NOT EXISTS ...
// 3) SELECT that database.
//
// At skeleton stage we only ensure database exists.
// Creating tables/migrations can be added later.

#include <string>

namespace dorm_alloc
{
    namespace infra
    {
        namespace db
        {

            class MySqlClient;

            class DbBootstrapper
            {
            public:
                static void EnsureDatabaseExists(MySqlClient &client, const std::string &db_name);
            };

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
