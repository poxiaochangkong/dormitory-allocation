#include "infrastructure/db/MySqlClient.h"

#include <stdexcept>

// mysql-connector-cpp headers are included in the header via:
//   <mysqlx/xdevapi.h>

namespace dorm_alloc
{
    namespace infra
    {
        namespace db
        {

            MySqlClient::MySqlClient(const dorm_alloc::infra::config::MySqlConfig &cfg)
                : cfg_(cfg)
            {
            }

            void MySqlClient::ConnectServer()
            {
                try
                {
                    // MySQL X DevAPI session connection string:
                    // mysqlx://user:password@host:port
                    const std::string uri =
                        "mysqlx://" + cfg_.user + ":" + cfg_.password +
                        "@" + cfg_.host + ":" + std::to_string(cfg_.port);

                    session_.reset(new mysqlx::Session(uri));
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Failed to connect to MySQL server: ") + e.what());
                }
            }

            void MySqlClient::UseDatabase(const std::string &db_name)
            {
                if (!session_)
                {
                    throw std::runtime_error("UseDatabase called before ConnectServer");
                }

                // In X DevAPI, schema can be chosen by sending SQL.
                // NOTE: Using plain SQL here is sufficient for bootstrap stage.
                Execute("USE `" + db_name + "`;");
            }

            void MySqlClient::Execute(const std::string &sql_text)
            {
                if (!session_)
                {
                    throw std::runtime_error("Execute called before ConnectServer");
                }

                try
                {
                    session_->sql(sql_text).execute();
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Failed to execute SQL: ") + e.what() + " | SQL=" + sql_text);
                }
            }

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
