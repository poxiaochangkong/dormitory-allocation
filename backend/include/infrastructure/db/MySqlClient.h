#pragma once

// A minimal MySQL client wrapper built on top of mysql-connector-cpp.
//
// IMPORTANT: mysql-connector-cpp in vcpkg provides the MySQL X DevAPI headers
// under `mysqlx/`.
//
// At skeleton stage we only need to execute plain SQL strings to:
// - create database if not exists
// - switch database
//
// So we wrap a `mysqlx::Session` and provide Execute/UseDatabase helpers.

#include <memory>
#include <string>

#include <mysqlx/xdevapi.h>

#include "infrastructure/config/AppConfig.h" // for MySqlConfig definition

namespace dorm_alloc
{
    namespace infra
    {
        namespace db
        {

            class MySqlClient
            {
            public:
                explicit MySqlClient(const dorm_alloc::infra::config::MySqlConfig &cfg);

                // Connect to MySQL server (without selecting a database).
                // This is used during bootstrap when the database may not exist yet.
                void ConnectServer();

                // Select a database after it is ensured to exist.
                void UseDatabase(const std::string &db_name);

                // Execute a statement that does not return a result set.
                void Execute(const std::string &sql);

            private:
                dorm_alloc::infra::config::MySqlConfig cfg_;
                std::unique_ptr<mysqlx::Session> session_;
            };

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
