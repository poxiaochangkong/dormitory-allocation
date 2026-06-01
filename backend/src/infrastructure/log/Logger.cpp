#include "infrastructure/log/Logger.h"

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

namespace dorm_alloc
{
    namespace infra
    {
        namespace log
        {

            // Map a level string to spdlog::level.
            static spdlog::level::level_enum ParseLevel(const std::string &level)
            {
                if (level == "trace")
                    return spdlog::level::trace;
                if (level == "debug")
                    return spdlog::level::debug;
                if (level == "info")
                    return spdlog::level::info;
                if (level == "warn")
                    return spdlog::level::warn;
                if (level == "error")
                    return spdlog::level::err;
                if (level == "critical")
                    return spdlog::level::critical;
                if (level == "off")
                    return spdlog::level::off;
                return spdlog::level::info;
            }

            // Create directory if it does not exist (single level).
            // For nested paths, this only creates the last component;
            // parent directories are assumed to exist or are created iteratively.
            static void EnsureDirExists(const std::string &dir)
            {
                if (dir.empty())
                    return;
                // Attempt to create; ignore error if it already exists.
                MKDIR(dir.c_str());
            }

            // Recursively ensure all path components exist.
            // Handles paths like "a/b/c" by creating "a", then "a/b", then "a/b/c".
            static void EnsureDirRecursive(std::string path)
            {
                if (path.empty())
                    return;

                // Normalize: remove trailing slash/backslash
                while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
                {
                    path.pop_back();
                }

                if (path.empty())
                    return;

                // Split by '/' and build progressively
                std::string current;
                for (size_t i = 0; i < path.size(); ++i)
                {
                    current += path[i];
                    if (path[i] == '/' || path[i] == '\\' || i == path.size() - 1)
                    {
                        EnsureDirExists(current);
                    }
                }
            }

            void InitLogger(const std::string &log_dir, const std::string &level)
            {
                try
                {
                    // Ensure log directory exists
                    EnsureDirRecursive(log_dir);

                    // Build the full log file path
                    std::string log_file = log_dir;
                    // Ensure trailing separator
                    if (!log_file.empty() && log_file.back() != '/' && log_file.back() != '\\')
                    {
                        log_file += '/';
                    }
                    log_file += "dorm_alloc.log";

                    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_file, 23, 59);

                    // Console pattern: shorter, easier to read during development
                    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

                    // File pattern: includes source location for debugging
                    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");

                    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
                    auto logger = std::make_shared<spdlog::logger>("dorm_alloc", sinks.begin(), sinks.end());

                    auto spdlog_level = ParseLevel(level);
                    logger->set_level(spdlog_level);

                    // Flush immediately on warn and above to avoid losing error logs on crash
                    logger->flush_on(spdlog::level::warn);

                    // Register as the default logger so SPDLOG_INFO etc. work everywhere
                    spdlog::set_default_logger(logger);

                    SPDLOG_INFO("Logger initialized. log_dir={}, level={}", log_dir, level);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "[Logger] Failed to initialize: " << e.what() << std::endl;
                }
            }

            void ShutdownLogger()
            {
                SPDLOG_INFO("Logger shutting down.");
                spdlog::shutdown();
            }

        } // namespace log
    } // namespace infra
} // namespace dorm_alloc