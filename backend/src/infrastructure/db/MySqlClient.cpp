//此文件由工具自动生成，请勿修改！
#include "infrastructure/db/MySqlClient.h"

#include <sstream>
#include <stdexcept>

// Classic API (JDBC / cppconn)
#include <mysql/jdbc.h>

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

            MySqlClient::~MySqlClient() = default;

            void MySqlClient::ConnectServer()
            {
                try
                {
                    // Classic API uses standard MySQL protocol (default 3306).
                    // URL format: tcp://host:port
                    std::ostringstream oss;
                    oss << "tcp://" << cfg_.host << ":" << cfg_.port;
                    const std::string url = oss.str();

                    driver_ = sql::mysql::get_mysql_driver_instance();
                    conn_.reset(driver_->connect(url, cfg_.user, cfg_.password));

                    // Optional but recommended.
                    conn_->setClientOption("OPT_RECONNECT", "true");
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Failed to connect to MySQL server: ") + e.what());
                }
            }

            void MySqlClient::UseDatabase(const std::string &db_name)
            {
                if (!conn_)
                {
                    throw std::runtime_error("UseDatabase called before ConnectServer");
                }

                try
                {
                    conn_->setSchema(db_name);
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Failed to select database: ") + e.what() + " | db=" + db_name);
                }
            }

            void MySqlClient::Execute(const std::string &sql_text)
            {
                if (!conn_)
                {
                    throw std::runtime_error("Execute called before ConnectServer");
                }

                try
                {
                    std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
                    stmt->execute(sql_text);
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Failed to execute SQL: ") + e.what() + " | SQL=" + sql_text);
                }
            }

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
