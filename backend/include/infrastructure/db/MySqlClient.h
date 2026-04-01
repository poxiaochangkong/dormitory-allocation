#pragma once

// A minimal MySQL client wrapper built on top of mysql-connector-cpp.
//
// IMPORTANT:
// We intentionally use **Classic API / JDBC (cppconn)** instead of X DevAPI.
// Reason: X DevAPI uses MySQL X Protocol which typically listens on port 33060,
// while Classic API connects to the normal MySQL port 3306.

#include <memory>
#include <string>

// Forward declarations for Classic API types.
// We keep heavy connector headers in the .cpp to reduce compile-time impact.
namespace sql
{
    class Connection;
}

namespace sql
{
    namespace mysql
    {
        class MySQL_Driver;
    }
}

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
                ~MySqlClient();

                // Connect to MySQL server (without selecting a database).
                // This is used during bootstrap when the database may not exist yet.
                void ConnectServer();

                // Select a database after it is ensured to exist.
                void UseDatabase(const std::string &db_name);

                // Execute a statement that does not return a result set.
                void Execute(const std::string &sql);

            private:
                dorm_alloc::infra::config::MySqlConfig cfg_;
                sql::mysql::MySQL_Driver *driver_ = nullptr; // non-owning singleton
                std::unique_ptr<sql::Connection> conn_;
            };

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
