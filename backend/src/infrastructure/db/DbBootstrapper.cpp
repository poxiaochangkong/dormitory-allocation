//此文件由工具自动生成，请勿修改
#include "infrastructure/db/DbBootstrapper.h"

#include "infrastructure/db/MySqlClient.h"

namespace dorm_alloc
{
    namespace infra
    {
        namespace db
        {

            void DbBootstrapper::EnsureDatabaseExists(MySqlClient &client, const std::string &db_name)
            {
                // Use utf8mb4 to properly store any Chinese / emoji / multilingual text.
                // MySQL 8.x default collation example: utf8mb4_0900_ai_ci
                const std::string sql =
                    "CREATE DATABASE IF NOT EXISTS `" + db_name +
                    "` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;";

                client.Execute(sql);
                client.UseDatabase(db_name);
            }

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
