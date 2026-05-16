#include "service/StudentService.h"

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

namespace dorm_alloc
{
    namespace service
    {

        // Generate a simple unique ID
        static std::string GenId()
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            int r = std::rand() % 10000;
            std::ostringstream oss;
            oss << "q_" << ms << "_" << r;
            return oss.str();
        }

        std::string StudentService::Login(
            MySqlClient &db,
            const std::string &student_no,
            const std::string &password)
        {
            auto rs = db.ExecuteQuery(
                "SELECT user_id, role, gender FROM `user` "
                "WHERE student_no = '" +
                student_no + "' AND password = '" + password + "';");

            if (!rs->next())
            {
                throw std::runtime_error("Invalid student number or password.");
            }

            nlohmann::json result;
            result["userId"] = rs->getString("user_id").asStdString();
            result["role"] = rs->getString("role").asStdString();
            result["gender"] = rs->getString("gender").asStdString();
            return result.dump();
        }

        std::string StudentService::SubmitQuestionnaire(
            MySqlClient &db,
            const std::string &user_id,
            const std::string &data_json)
        {
            auto data = nlohmann::json::parse(data_json);

            // 1. Update user basic info if provided
            if (data.contains("basicInfo"))
            {
                auto &info = data["basicInfo"];
                std::string gender = info.value("gender", "");
                std::string college = info.value("college", "");
                std::string major = info.value("major", "");
                std::string grade = info.value("grade", "");
                int dorm_type = info.value("dormType", 4);

                std::ostringstream sql;
                sql << "UPDATE `user` SET "
                    << "gender = '" << gender << "', "
                    << "college = '" << college << "', "
                    << "major = '" << major << "', "
                    << "grade = '" << grade << "', "
                    << "dorm_type = " << dorm_type << " "
                    << "WHERE user_id = '" << user_id << "';";
                db.Execute(sql.str());
            }

            // 2. Save questionnaire data
            std::string qid = GenId();
            if (data.contains("questionnaire"))
            {
                auto &q = data["questionnaire"];
                std::ostringstream sql;
                sql << "INSERT INTO questionnaire "
                    << "(questionnaire_id, user_id, sleep_schedule, hygiene_level, "
                    << "noise_tolerance, temperature_preference, social_preference, "
                    << "gaming_behavior, mbti_type, raw_answers) VALUES ("
                    << "'" << qid << "', "
                    << "'" << user_id << "', "
                    << "'" << q.value("sleepSchedule", "") << "', "
                    << q.value("hygieneLevel", 0) << ", "
                    << q.value("noiseTolerance", 0) << ", "
                    << q.value("temperaturePreference", 0) << ", "
                    << q.value("socialPreference", 0) << ", "
                    << "'" << q.value("gamingBehavior", "") << "', "
                    << "'" << q.value("mbtiType", "") << "', "
                    << "'" << q.dump() << "');";
                db.Execute(sql.str());
            }

            // 3. Save preference weights if provided
            if (data.contains("preference"))
            {
                auto &pref = data["preference"];
                std::string pref_id = "pref_" + user_id;
                std::ostringstream sql;
                sql << "INSERT INTO preference "
                    << "(preference_id, user_id, similarity_weight, "
                    << "complementarity_weight, veto_safety_weight) VALUES ("
                    << "'" << pref_id << "', "
                    << "'" << user_id << "', "
                    << pref.value("similarityWeight", 0.5) << ", "
                    << pref.value("complementarityWeight", 0.2) << ", "
                    << pref.value("vetoSafetyWeight", 0.3) << ") "
                    << "ON DUPLICATE KEY UPDATE "
                    << "similarity_weight = VALUES(similarity_weight), "
                    << "complementarity_weight = VALUES(complementarity_weight), "
                    << "veto_safety_weight = VALUES(veto_safety_weight);";
                db.Execute(sql.str());
            }

            // 4. Save veto items if provided
            if (data.contains("vetoItems") && data["vetoItems"].is_array())
            {
                // Delete existing veto items for this user
                db.Execute("DELETE FROM veto WHERE user_id = '" + user_id + "';");

                for (const auto &item : data["vetoItems"])
                {
                    std::string veto_id = GenId();
                    std::ostringstream sql;
                    sql << "INSERT INTO veto (veto_id, user_id, veto_item) VALUES ("
                        << "'" << veto_id << "', "
                        << "'" << user_id << "', "
                        << "'" << item.get<std::string>() << "');";
                    db.Execute(sql.str());
                }
            }

            // 5. Save open text profile if provided
            if (data.contains("openText"))
            {
                auto &ot = data["openText"];
                std::string profile_id = "prof_" + user_id;
                std::ostringstream sql;
                sql << "INSERT INTO open_text_profile "
                    << "(profile_id, user_id, self_description, roommate_expectation) VALUES ("
                    << "'" << profile_id << "', "
                    << "'" << user_id << "', "
                    << "'" << ot.value("selfDescription", "") << "', "
                    << "'" << ot.value("roommateExpectation", "") << "') "
                    << "ON DUPLICATE KEY UPDATE "
                    << "self_description = VALUES(self_description), "
                    << "roommate_expectation = VALUES(roommate_expectation);";
                db.Execute(sql.str());
            }

            nlohmann::json result;
            result["questionnaireId"] = qid;
            return result.dump();
        }

        std::string StudentService::GetMatchResult(
            MySqlClient &db,
            const std::string &user_id)
        {
            auto rs = db.ExecuteQuery(
                "SELECT mr.result_id, mr.task_id, mr.dorm_id, mr.roommate_ids, "
                "  mr.total_score, mr.similarity_score, mr.complementarity_score, "
                "  mr.veto_risk_score, mr.explanation_text, "
                "  d.building, d.room_number "
                "FROM match_result mr "
                "LEFT JOIN dormitory d ON mr.dorm_id = d.dorm_id "
                "WHERE mr.user_id = '" +
                user_id + "' ORDER BY mr.result_id DESC LIMIT 1;");

            if (!rs->next())
            {
                // No result yet
                return "{}";
            }

            nlohmann::json result;
            result["resultId"] = rs->getString("result_id").asStdString();
            result["taskId"] = rs->getString("task_id").asStdString();
            result["dormId"] = rs->getString("dorm_id").asStdString();
            result["building"] = rs->getString("building").asStdString();
            result["roomNumber"] = rs->getString("room_number").asStdString();

            // Parse roommate_ids (comma-separated)
            std::string roommate_str = rs->getString("roommate_ids").asStdString();
            if (!roommate_str.empty())
            {
                std::vector<std::string> roommates;
                std::istringstream iss(roommate_str);
                std::string tok;
                while (std::getline(iss, tok, ','))
                {
                    roommates.push_back(tok);
                }
                result["roommateIds"] = roommates;
            }

            result["totalScore"] = rs->getDouble("total_score");
            result["similarityScore"] = rs->getDouble("similarity_score");
            result["complementarityScore"] = rs->getDouble("complementarity_score");
            result["vetoRiskScore"] = rs->getDouble("veto_risk_score");
            result["explanationText"] = rs->getString("explanation_text").asStdString();

            // Query roommate details
            if (result.contains("roommateIds") && result["roommateIds"].is_array())
            {
                nlohmann::json roommate_details = nlohmann::json::array();
                for (const auto &rid : result["roommateIds"])
                {
                    auto detail_rs = db.ExecuteQuery(
                        "SELECT user_id, student_no, gender, college, major "
                        "FROM `user` WHERE user_id = '" +
                        rid.get<std::string>() + "';");
                    if (detail_rs->next())
                    {
                        nlohmann::json rm;
                        rm["userId"] = detail_rs->getString("user_id").asStdString();
                        rm["gender"] = detail_rs->getString("gender").asStdString();
                        rm["college"] = detail_rs->getString("college").asStdString();
                        rm["major"] = detail_rs->getString("major").asStdString();
                        roommate_details.push_back(rm);
                    }
                }
                result["roommates"] = roommate_details;
            }

            return result.dump();
        }

        std::string StudentService::GetQuestionnaireTemplate()
        {
            nlohmann::json tmpl;
            tmpl["sleepScheduleOptions"] = {"early", "normal", "late"};
            tmpl["hygieneLevelRange"] = {1, 5};
            tmpl["noiseToleranceRange"] = {1, 5};
            tmpl["temperaturePreferenceRange"] = {18, 30};
            tmpl["socialPreferenceRange"] = {1, 5};
            tmpl["gamingBehaviorOptions"] = {"never", "sometimes", "often", "always"};
            tmpl["mbtiTypes"] = {"INTJ", "INTP", "ENTJ", "ENTP",
                                 "INFJ", "INFP", "ENFJ", "ENFP",
                                 "ISTJ", "ISFJ", "ESTJ", "ESFJ",
                                 "ISTP", "ISFP", "ESTP", "ESFP"};
            tmpl["vetoItemOptions"] = {"often", "late", "never"};
            return tmpl.dump();
        }

    } // namespace service
} // namespace dorm_alloc