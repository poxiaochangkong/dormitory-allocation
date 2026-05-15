#pragma once

// This header defines the application configuration model.
//
// Why do we keep config in a dedicated module?
// - Both the HTTP server and the CLI tool need the same configuration.
// - We want to avoid hard-coding passwords/ports in source code.
// - In the future, config can be extended (logging level, JWT secret, etc.).

#include <cstdint>
#include <string>

// C++14 compatibility: do NOT use C++17 nested-namespace definition syntax.
namespace dorm_alloc
{
    namespace infra
    {
        namespace config
        {

            struct HttpConfig
            {
                std::string host = "0.0.0.0";
                std::uint16_t port = 8080;
            };

            struct MySqlConfig
            {
                std::string host = "127.0.0.1";
                std::uint16_t port = 3306;
                std::string user = "root";
                std::string password = "root";

                // Database name used by the application.
                // Requirement: if it doesn't exist, we create it during startup.
                std::string database = "dorm_alloc";
            };

            struct AppConfig
            {
                HttpConfig http;
                MySqlConfig mysql;
            };

            // Load config from a JSON file.
            // Throws std::runtime_error if the file cannot be read or JSON is invalid.
            AppConfig LoadFromJsonFile(const std::string &path);

        } // namespace config
    } // namespace infra
} // namespace dorm_alloc
