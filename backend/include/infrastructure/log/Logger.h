#pragma once

// Centralized logging module using spdlog.
//
// Usage:
//   #include "infrastructure/log/Logger.h"
//   // Then use the global macros:
//   LOG_INFO("Server started on port {}", port);
//   LOG_WARN("User not found: {}", user_id);
//   LOG_ERROR("Database error: {}", e.what());
//   LOG_DEBUG("SQL: {}", sql);
//
// Initialization:
//   dorm_alloc::infra::log::InitLogger(log_dir, log_level);
//   // ... at shutdown ...
//   dorm_alloc::infra::log::ShutdownLogger();

#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace dorm_alloc
{
    namespace infra
    {
        namespace log
        {

            // Initialize spdlog with daily rotating file sink + console sink.
            // log_dir:  directory for log files (will be created if missing).
            //           The logger stores files at log_dir/dorm_alloc.log.
            // level:    log level string ("trace", "debug", "info", "warn", "error").
            void InitLogger(const std::string &log_dir, const std::string &level);

            // Flush all loggers and release spdlog resources.
            void ShutdownLogger();

        } // namespace log
    } // namespace infra
} // namespace dorm_alloc

// Global logging macros — use these everywhere for convenience.
#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)