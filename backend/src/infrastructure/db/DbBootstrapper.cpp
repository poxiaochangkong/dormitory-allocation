#include "infrastructure/db/DbBootstrapper.h"

#include <mysql/jdbc.h>
#include <sstream>

#include "infrastructure/db/MySqlClient.h"
#include "infrastructure/auth/CryptoUtil.h"
#include "infrastructure/log/Logger.h"

namespace dorm_alloc
{
    namespace infra
    {
        namespace db
        {

            void DbBootstrapper::EnsureDatabaseExists(MySqlClient &client, const std::string &db_name)
            {
                // Use utf8mb4 to properly store any Chinese / emoji / multilingual text.
                const std::string sql =
                    "CREATE DATABASE IF NOT EXISTS `" + db_name +
                    "` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;";

                client.Execute(sql);
                client.UseDatabase(db_name);
            }

            void DbBootstrapper::EnsureTablesExist(MySqlClient &client)
            {
                // 1. user table (password stores SHA-256 hash, salt stores per-user random salt)
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `user` ("
                    "  user_id VARCHAR(64) PRIMARY KEY,"
                    "  student_no VARCHAR(32) NOT NULL UNIQUE,"
                    "  password VARCHAR(128) NOT NULL,"
                    "  salt VARCHAR(64) NOT NULL DEFAULT '',"
                    "  gender VARCHAR(16) NOT NULL DEFAULT '',"
                    "  college VARCHAR(64) NOT NULL DEFAULT '',"
                    "  major VARCHAR(64) NOT NULL DEFAULT '',"
                    "  grade VARCHAR(16),"
                    "  dorm_type INT DEFAULT 4,"
                    "  role VARCHAR(16) NOT NULL DEFAULT 'student',"
                    "  token VARCHAR(128),"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // Ensure salt column exists for databases created before this migration
                try
                {
                    client.Execute("ALTER TABLE `user` ADD COLUMN salt VARCHAR(64) NOT NULL DEFAULT '' AFTER password;");
                }
                catch (const std::exception &)
                {
                    // Column already exists — ignore
                }

                // Ensure token column exists for databases created before this migration
                try
                {
                    client.Execute("ALTER TABLE `user` ADD COLUMN token VARCHAR(128) AFTER role;");
                }
                catch (const std::exception &)
                {
                    // Column already exists — ignore
                }

                // 2. questionnaire table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `questionnaire` ("
                    "  questionnaire_id VARCHAR(64) PRIMARY KEY,"
                    "  user_id VARCHAR(64) NOT NULL,"
                    "  sleep_schedule VARCHAR(32),"
                    "  hygiene_level INT,"
                    "  noise_tolerance INT,"
                    "  temperature_preference INT,"
                    "  social_preference INT,"
                    "  gaming_behavior VARCHAR(32),"
                    "  mbti_type VARCHAR(16),"
                    "  raw_answers TEXT,"
                    "  submitted_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "  FOREIGN KEY (user_id) REFERENCES `user`(user_id)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 3. preference table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `preference` ("
                    "  preference_id VARCHAR(64) PRIMARY KEY,"
                    "  user_id VARCHAR(64) NOT NULL UNIQUE,"
                    "  similarity_weight DOUBLE DEFAULT 0.5,"
                    "  complementarity_weight DOUBLE DEFAULT 0.2,"
                    "  veto_safety_weight DOUBLE DEFAULT 0.3,"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "  FOREIGN KEY (user_id) REFERENCES `user`(user_id)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 4. veto table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `veto` ("
                    "  veto_id VARCHAR(64) PRIMARY KEY,"
                    "  user_id VARCHAR(64) NOT NULL,"
                    "  veto_item VARCHAR(128) NOT NULL,"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "  FOREIGN KEY (user_id) REFERENCES `user`(user_id)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 5. open_text_profile table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `open_text_profile` ("
                    "  profile_id VARCHAR(64) PRIMARY KEY,"
                    "  user_id VARCHAR(64) NOT NULL UNIQUE,"
                    "  self_description TEXT,"
                    "  roommate_expectation TEXT,"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "  FOREIGN KEY (user_id) REFERENCES `user`(user_id)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 6. dormitory table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `dormitory` ("
                    "  dorm_id VARCHAR(64) PRIMARY KEY,"
                    "  building VARCHAR(32) NOT NULL,"
                    "  room_number VARCHAR(16) NOT NULL,"
                    "  capacity INT NOT NULL DEFAULT 4,"
                    "  gender VARCHAR(16),"
                    "  college VARCHAR(64),"
                    "  is_available TINYINT NOT NULL DEFAULT 1,"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 7. allocation_task table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `allocation_task` ("
                    "  task_id VARCHAR(64) PRIMARY KEY,"
                    "  task_name VARCHAR(128) NOT NULL,"
                    "  college VARCHAR(64),"
                    "  major VARCHAR(64),"
                    "  gender VARCHAR(16),"
                    "  status VARCHAR(32) NOT NULL DEFAULT 'pending',"
                    "  rule_config TEXT,"
                    "  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "  executed_at DATETIME,"
                    "  finished_at DATETIME"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 8. match_result table
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `match_result` ("
                    "  result_id VARCHAR(64) PRIMARY KEY,"
                    "  task_id VARCHAR(64) NOT NULL,"
                    "  user_id VARCHAR(64) NOT NULL,"
                    "  dorm_id VARCHAR(64) NOT NULL,"
                    "  roommate_ids TEXT,"
                    "  total_score DOUBLE,"
                    "  similarity_score DOUBLE,"
                    "  complementarity_score DOUBLE,"
                    "  veto_risk_score DOUBLE,"
                    "  explanation_text TEXT,"
                    "  FOREIGN KEY (task_id) REFERENCES allocation_task(task_id),"
                    "  FOREIGN KEY (user_id) REFERENCES `user`(user_id),"
                    "  FOREIGN KEY (dorm_id) REFERENCES dormitory(dorm_id)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // 9. system_config table (key-value store for allocation rules, etc.)
                client.Execute(
                    "CREATE TABLE IF NOT EXISTS `system_config` ("
                    "  config_key VARCHAR(128) PRIMARY KEY,"
                    "  config_value TEXT NOT NULL,"
                    "  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;");

                // Create indexes (MySQL does not support IF NOT EXISTS for CREATE INDEX,
                // so we silently ignore errors since indexes may already exist)
                const char *indexes[] = {
                    "ALTER TABLE `user` ADD INDEX idx_user_college_major_gender (college, major, gender);",
                    "ALTER TABLE questionnaire ADD INDEX idx_questionnaire_user (user_id);",
                    "ALTER TABLE veto ADD INDEX idx_veto_user (user_id);",
                    "ALTER TABLE allocation_task ADD INDEX idx_task_status (college, major, status);",
                    "ALTER TABLE match_result ADD INDEX idx_result_task_dorm (task_id, dorm_id);",
                };
                for (const char *idx_sql : indexes)
                {
                    try
                    {
                        client.Execute(idx_sql);
                    }
                    catch (const std::exception &)
                    {
                        // Index already exists — ignore
                    }
                }

                // Seed default admin account if no admin exists.
                // Ensures exactly one admin with hashed password.
                // Default credentials: admin / admin123
                try
                {
                    auto rs = client.ExecuteQuery(
                        "SELECT COUNT(*) AS cnt FROM `user` WHERE role = 'admin';");
                    if (rs->next() && rs->getInt("cnt") == 0)
                    {
                        std::string admin_salt = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
                        std::string admin_hash = dorm_alloc::infra::auth::CryptoUtil::HashPassword("admin123", admin_salt);
                        std::ostringstream sql;
                        sql << "INSERT INTO `user`"
                            << "  (user_id, student_no, password, salt, gender, college, major, grade, role)"
                            << "  VALUES ('admin_001', 'admin', '"
                            << admin_hash << "', '"
                            << admin_salt << "', "
                            << "'male', 'System', 'Admin', '2024', 'admin');";
                        client.Execute(sql.str());
                    }
                }
                catch (const std::exception &)
                {
                    // Seed insertion failed (e.g. duplicate key) — ignore
                }

                // Migrate existing admin password from plaintext to hashed if needed.
                // Old versions stored plaintext password with empty salt.
                try
                {
                    auto rs = client.ExecuteQuery(
                        "SELECT user_id, salt FROM `user` WHERE role = 'admin' AND (salt IS NULL OR salt = '');");
                    if (rs->next())
                    {
                        std::string uid = rs->getString("user_id").asStdString();
                        std::string new_salt = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
                        std::string new_hash = dorm_alloc::infra::auth::CryptoUtil::HashPassword("admin123", new_salt);
                        std::ostringstream sql;
                        sql << "UPDATE `user` SET password = '" << new_hash
                            << "', salt = '" << new_salt
                            << "' WHERE user_id = '" << uid << "';";
                        client.Execute(sql.str());
                    }
                }
                catch (const std::exception &)
                {
                    // Migration failed — ignore
                }
            }

        } // namespace db
    } // namespace infra
} // namespace dorm_alloc
