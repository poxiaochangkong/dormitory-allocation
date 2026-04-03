//此文件由工具自动生成，请勿直接修改！

#include "infrastructure/config/AppConfig.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

// C++14 compatible namespace blocks.
namespace dorm_alloc
{
    namespace infra
    {
        namespace config
        {

            static std::string ReadAllText(const std::string &path)
            {
                std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
                if (!ifs)
                {
                    throw std::runtime_error("Failed to open config file: " + path);
                }
                std::ostringstream oss;
                oss << ifs.rdbuf();
                return oss.str();
            }

            AppConfig LoadFromJsonFile(const std::string &path)
            {
                const auto text = ReadAllText(path);

                nlohmann::json j;
                try
                {
                    j = nlohmann::json::parse(text);
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error(std::string("Invalid JSON in config file: ") + e.what());
                }

                AppConfig cfg;

                // http
                if (j.contains("http"))
                {
                    const auto &h = j.at("http");
                    if (h.contains("host"))
                        cfg.http.host = h.at("host").get<std::string>();
                    if (h.contains("port"))
                        cfg.http.port = static_cast<std::uint16_t>(h.at("port").get<int>());
                }

                // mysql
                if (j.contains("mysql"))
                {
                    const auto &m = j.at("mysql");
                    if (m.contains("host"))
                        cfg.mysql.host = m.at("host").get<std::string>();
                    if (m.contains("port"))
                        cfg.mysql.port = static_cast<std::uint16_t>(m.at("port").get<int>());
                    if (m.contains("user"))
                        cfg.mysql.user = m.at("user").get<std::string>();
                    if (m.contains("password"))
                        cfg.mysql.password = m.at("password").get<std::string>();
                    if (m.contains("database"))
                        cfg.mysql.database = m.at("database").get<std::string>();
                }

                return cfg;
            }

        } // namespace config
    } // namespace infra
} // namespace dorm_alloc
