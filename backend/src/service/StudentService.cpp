#include "service/StudentService.h"

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

#include "infrastructure/auth/CryptoUtil.h"

namespace dorm_alloc
{
    namespace service
    {

        // Generate a unique ID with a given prefix
        static std::string GenId(const std::string &prefix)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            int r = std::rand() % 10000;
            std::ostringstream oss;
            oss << prefix << ms << "_" << r;
            return oss.str();
        }

        // Escape single quotes for SQL string values
        static std::string Escape(const std::string &s)
        {
            std::string result;
            result.reserve(s.size());
            for (char c : s)
            {
                if (c == '\'')
                    result += "''";
                else
                    result += c;
            }
            return result;
        }

        // Map frontend letter codes to backend sleep schedule values
        static std::string MapSleepSchedule(const std::string &code)
        {
            if (code == "A")
                return "early";
            if (code == "B")
                return "normal";
            if (code == "C")
                return "late";
            if (code == "D")
                return "very_late";
            return code; // already in backend format or empty
        }

        // Map frontend letter codes to numeric levels (1-5)
        static int MapToLevel(const std::string &code, int default_val = 3)
        {
            if (code == "A")
                return 5;
            if (code == "B")
                return 3;
            if (code == "C")
                return 1;
            // If it's already a number string, parse it
            try
            {
                return std::stoi(code);
            }
            catch (...)
            {
            }
            return default_val;
        }

        // Map frontend social energy code to social preference numeric
        static int MapSocialPreference(const std::string &code)
        {
            if (code == "A")
                return 5; // social butterfly
            if (code == "B")
                return 3; // balanced
            if (code == "C")
                return 1; // lone wolf
            return MapToLevel(code, 3);
        }

        // Generate a simple random token
        std::string StudentService::GenerateToken()
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            std::srand(static_cast<unsigned>(ms));
            const char hex[] = "0123456789abcdef";
            std::string token = "tk_";
            for (int i = 0; i < 32; ++i)
            {
                token += hex[std::rand() % 16];
            }
            return token;
        }

        // ---- Login ----
        std::string StudentService::Login(
            MySqlClient &db,
            const std::string &student_no,
            const std::string &password)
        {
            // Look up user by student_no, retrieve stored hash and salt
            auto rs = db.ExecuteQuery(
                "SELECT user_id, role, gender, college, major, grade, dorm_type, password, salt "
                "FROM `user` "
                "WHERE student_no = '" +
                Escape(student_no) + "';");

            if (!rs->next())
            {
                throw std::runtime_error("Invalid student number or password.");
            }

            // Verify password hash
            std::string stored_hash = rs->getString("password").asStdString();
            std::string salt = rs->getString("salt").asStdString();

            if (!dorm_alloc::infra::auth::CryptoUtil::VerifyPassword(password, salt, stored_hash))
            {
                throw std::runtime_error("Invalid student number or password.");
            }

            // Generate a token for authentication
            std::string token = GenerateToken();
            std::string user_id = rs->getString("user_id").asStdString();

            // Store token in database (update the user record)
            db.Execute(
                "UPDATE `user` SET token = '" + Escape(token) +
                "' WHERE user_id = '" + Escape(user_id) + "';");

            nlohmann::json result;
            result["userId"] = user_id;
            result["studentNo"] = student_no;
            result["role"] = rs->getString("role").asStdString();
            result["gender"] = rs->getString("gender").asStdString();
            result["college"] = rs->getString("college").asStdString();
            result["major"] = rs->getString("major").asStdString();
            result["grade"] = rs->getString("grade").asStdString();
            result["dormType"] = rs->getInt("dorm_type");
            result["token"] = token;
            return result.dump();
        }

        // ---- GetStudentInfo ----
        std::string StudentService::GetStudentInfo(
            MySqlClient &db,
            const std::string &user_id)
        {
            auto rs = db.ExecuteQuery(
                "SELECT user_id, student_no, gender, college, major, grade, dorm_type "
                "FROM `user` WHERE user_id = '" +
                Escape(user_id) + "';");

            if (!rs->next())
            {
                throw std::runtime_error("User not found: " + user_id);
            }

            nlohmann::json result;
            result["userId"] = rs->getString("user_id").asStdString();
            result["studentNo"] = rs->getString("student_no").asStdString();
            result["gender"] = rs->getString("gender").asStdString();
            result["college"] = rs->getString("college").asStdString();
            result["major"] = rs->getString("major").asStdString();
            result["grade"] = rs->getString("grade").asStdString();
            result["dormType"] = rs->getInt("dorm_type");
            return result.dump();
        }

        // ---- GetQuestionnaireStatus ----
        std::string StudentService::GetQuestionnaireStatus(
            MySqlClient &db,
            const std::string &user_id)
        {
            auto rs = db.ExecuteQuery(
                "SELECT questionnaire_id FROM questionnaire "
                "WHERE user_id = '" +
                Escape(user_id) + "' LIMIT 1;");

            nlohmann::json result;
            if (rs->next())
            {
                result["submitted"] = true;
                result["questionnaireId"] = rs->getString("questionnaire_id").asStdString();
            }
            else
            {
                result["submitted"] = false;
                result["questionnaireId"] = nullptr;
            }
            return result.dump();
        }

        // ---- SubmitQuestionnaire ----
        // Accepts both frontend format and original backend format.
        //
        // Frontend format:
        // {
        //   "basicInfo": { "gender":"male", "college":"...", "major":"..." },
        //   "traditionalHabits": { "q02_sleepTime":"A", "q03_wakeTime":"B", ... },
        //   "vetoSettings": { "v11_smokeAlcohol":true, "v12_midnightGaming":true, ... },
        //   "personality": { "p21_mbti":"INTJ", "p22_socialEnergy":"A", ... }
        // }
        //
        // Backend format (also accepted):
        // {
        //   "basicInfo": { ... },
        //   "questionnaire": { "sleepSchedule":"early", "hygieneLevel":3, ... },
        //   "preference": { "similarityWeight":0.5, ... },
        //   "vetoItems": ["smoke", "midnight_gaming"],
        //   "openText": { "selfDescription":"...", "roommateExpectation":"..." }
        // }
        std::string StudentService::SubmitQuestionnaire(
            MySqlClient &db,
            const std::string &user_id,
            const std::string &data_json)
        {
            auto data = nlohmann::json::parse(data_json);

            // Determine if this is frontend format or backend format
            bool is_frontend_format = data.contains("traditionalHabits") ||
                                      data.contains("vetoSettings") ||
                                      data.contains("personality");

            // ===== 1. Update user basic info =====
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
                    << "gender = '" << Escape(gender) << "', "
                    << "college = '" << Escape(college) << "', "
                    << "major = '" << Escape(major) << "', "
                    << "grade = '" << Escape(grade) << "', "
                    << "dorm_type = " << dorm_type << " "
                    << "WHERE user_id = '" << Escape(user_id) << "';";
                db.Execute(sql.str());
            }

            // ===== 2. Build questionnaire data =====
            std::string sleep_schedule;
            int hygiene_level = 3;
            int noise_tolerance = 3;
            int temperature_preference = 24;
            int social_preference = 3;
            std::string gaming_behavior;
            std::string mbti_type;
            nlohmann::json raw_answers;

            if (is_frontend_format)
            {
                // Map frontend traditionalHabits to backend fields
                if (data.contains("traditionalHabits"))
                {
                    auto &th = data["traditionalHabits"];
                    sleep_schedule = MapSleepSchedule(th.value("q02_sleepTime", ""));
                    // q03_wakeTime: store in raw_answers but no direct backend field
                    hygiene_level = MapToLevel(th.value("q06_hygiene", "B"));
                    // q07_laundry, q08_studyPlace: store in raw_answers
                }

                // Map frontend personality to backend fields
                if (data.contains("personality"))
                {
                    auto &p = data["personality"];
                    mbti_type = p.value("p21_mbti", "");
                    social_preference = MapSocialPreference(p.value("p22_socialEnergy", "B"));
                }

                // Store the entire original data as raw_answers
                raw_answers = data;
            }
            else
            {
                // Direct backend format
                if (data.contains("questionnaire"))
                {
                    auto &q = data["questionnaire"];
                    sleep_schedule = q.value("sleepSchedule", "");
                    hygiene_level = q.value("hygieneLevel", 3);
                    noise_tolerance = q.value("noiseTolerance", 3);
                    temperature_preference = q.value("temperaturePreference", 24);
                    social_preference = q.value("socialPreference", 3);
                    gaming_behavior = q.value("gamingBehavior", "");
                    mbti_type = q.value("mbtiType", "");
                    raw_answers = q;
                }
            }

            // Save questionnaire record
            std::string qid = GenId("q_");
            {
                std::ostringstream sql;
                sql << "INSERT INTO questionnaire "
                    << "(questionnaire_id, user_id, sleep_schedule, hygiene_level, "
                    << "noise_tolerance, temperature_preference, social_preference, "
                    << "gaming_behavior, mbti_type, raw_answers) VALUES ("
                    << "'" << qid << "', "
                    << "'" << Escape(user_id) << "', "
                    << "'" << Escape(sleep_schedule) << "', "
                    << hygiene_level << ", "
                    << noise_tolerance << ", "
                    << temperature_preference << ", "
                    << social_preference << ", "
                    << "'" << Escape(gaming_behavior) << "', "
                    << "'" << Escape(mbti_type) << "', "
                    << "'" << Escape(raw_answers.dump()) << "');";
                db.Execute(sql.str());
            }

            // ===== 3. Save preference weights =====
            {
                double sim_w = 0.5, comp_w = 0.2, veto_w = 0.3;
                if (data.contains("preference"))
                {
                    auto &pref = data["preference"];
                    sim_w = pref.value("similarityWeight", 0.5);
                    comp_w = pref.value("complementarityWeight", 0.2);
                    veto_w = pref.value("vetoSafetyWeight", 0.3);
                }
                else if (is_frontend_format)
                {
                    // Use default weights when frontend doesn't provide them
                    sim_w = 0.5;
                    comp_w = 0.25;
                    veto_w = 0.25;
                }

                std::string pref_id = "pref_" + user_id;
                std::ostringstream sql;
                sql << "INSERT INTO preference "
                    << "(preference_id, user_id, similarity_weight, "
                    << "complementarity_weight, veto_safety_weight) VALUES ("
                    << "'" << pref_id << "', "
                    << "'" << Escape(user_id) << "', "
                    << sim_w << ", "
                    << comp_w << ", "
                    << veto_w << ") "
                    << "ON DUPLICATE KEY UPDATE "
                    << "similarity_weight = VALUES(similarity_weight), "
                    << "complementarity_weight = VALUES(complementarity_weight), "
                    << "veto_safety_weight = VALUES(veto_safety_weight);";
                db.Execute(sql.str());
            }

            // ===== 4. Save veto items =====
            {
                std::vector<std::string> veto_list;

                if (is_frontend_format && data.contains("vetoSettings"))
                {
                    // Convert frontend boolean switches to string array
                    auto &vs = data["vetoSettings"];
                    // Map each boolean switch to a veto item string
                    if (vs.value("v11_smokeAlcohol", false))
                        veto_list.push_back("smoke_alcohol");
                    if (vs.value("v12_midnightGaming", false))
                        veto_list.push_back("midnight_gaming");
                    if (vs.value("v13_loudSpeaker", false))
                        veto_list.push_back("loud_speaker");
                    if (vs.value("v14_oppositeSex", false))
                        veto_list.push_back("opposite_sex");
                    if (vs.value("v15_badHygiene", false))
                        veto_list.push_back("bad_hygiene");
                    if (vs.value("v16_overDemand", false))
                        veto_list.push_back("over_demand");
                    if (vs.value("v17_boundary", false))
                        veto_list.push_back("boundary_violation");
                    if (vs.value("v19_pets", false))
                        veto_list.push_back("pets");
                }
                else if (data.contains("vetoItems") && data["vetoItems"].is_array())
                {
                    for (const auto &item : data["vetoItems"])
                    {
                        veto_list.push_back(item.get<std::string>());
                    }
                }

                // Delete existing veto items and insert new ones
                db.Execute("DELETE FROM veto WHERE user_id = '" + Escape(user_id) + "';");
                for (const auto &item : veto_list)
                {
                    std::string veto_id = GenId("v_");
                    std::ostringstream sql;
                    sql << "INSERT INTO veto (veto_id, user_id, veto_item) VALUES ("
                        << "'" << veto_id << "', "
                        << "'" << Escape(user_id) << "', "
                        << "'" << Escape(item) << "');";
                    db.Execute(sql.str());
                }
            }

            // ===== 5. Save open text profile =====
            if (data.contains("openText"))
            {
                auto &ot = data["openText"];
                std::string profile_id = "prof_" + user_id;
                std::ostringstream sql;
                sql << "INSERT INTO open_text_profile "
                    << "(profile_id, user_id, self_description, roommate_expectation) VALUES ("
                    << "'" << profile_id << "', "
                    << "'" << Escape(user_id) << "', "
                    << "'" << Escape(ot.value("selfDescription", "")) << "', "
                    << "'" << Escape(ot.value("roommateExpectation", "")) << "') "
                    << "ON DUPLICATE KEY UPDATE "
                    << "self_description = VALUES(self_description), "
                    << "roommate_expectation = VALUES(roommate_expectation);";
                db.Execute(sql.str());
            }

            nlohmann::json result;
            result["questionnaireId"] = qid;
            return result.dump();
        }

        // ---- SubmitSceneData ----
        // Accepts immersive scene data from the frontend's ImmersiveScene.vue
        // Stores the data in the questionnaire's raw_answers field (appended)
        // or in open_text_profile if descriptive.
        std::string StudentService::SubmitSceneData(
            MySqlClient &db,
            const std::string &user_id,
            const std::string &data_json)
        {
            auto data = nlohmann::json::parse(data_json);

            // Store scene data in open_text_profile or a separate mechanism.
            // For now, we update the existing questionnaire's raw_answers
            // and also update the open_text_profile if scene provides text fields.
            std::string self_desc = data.value("selfDescription", "");
            std::string roommate_exp = data.value("roommateExpectation", "");

            if (!self_desc.empty() || !roommate_exp.empty())
            {
                std::string profile_id = "prof_" + user_id;
                std::ostringstream sql;
                sql << "INSERT INTO open_text_profile "
                    << "(profile_id, user_id, self_description, roommate_expectation) VALUES ("
                    << "'" << profile_id << "', "
                    << "'" << Escape(user_id) << "', "
                    << "'" << Escape(self_desc) << "', "
                    << "'" << Escape(roommate_exp) << "') "
                    << "ON DUPLICATE KEY UPDATE "
                    << "self_description = COALESCE(NULLIF(VALUES(self_description), ''), self_description), "
                    << "roommate_expectation = COALESCE(NULLIF(VALUES(roommate_expectation), ''), roommate_expectation);";
                db.Execute(sql.str());
            }

            // Also store the full scene data as raw scene data
            // by updating the questionnaire raw_answers with scene info
            auto rs = db.ExecuteQuery(
                "SELECT questionnaire_id, raw_answers FROM questionnaire "
                "WHERE user_id = '" +
                Escape(user_id) + "' ORDER BY questionnaire_id DESC LIMIT 1;");

            if (rs->next())
            {
                std::string qid = rs->getString("questionnaire_id").asStdString();
                // Merge scene data into existing raw_answers
                nlohmann::json existing;
                try
                {
                    existing = nlohmann::json::parse(rs->getString("raw_answers").asStdString());
                }
                catch (...)
                {
                    existing = nlohmann::json::object();
                }
                existing["sceneData"] = data;

                std::ostringstream sql;
                sql << "UPDATE questionnaire SET raw_answers = '"
                    << Escape(existing.dump())
                    << "' WHERE questionnaire_id = '" << Escape(qid) << "';";
                db.Execute(sql.str());
            }

            nlohmann::json result;
            result["status"] = "ok";
            return result.dump();
        }

        // ---- GetMatchResult ----
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
                Escape(user_id) + "' ORDER BY mr.result_id DESC LIMIT 1;");

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

            // Scores - 3 from algorithm + 2 derived
            double sim_score = rs->getDouble("similarity_score");
            double comp_score = rs->getDouble("complementarity_score");
            double veto_score = rs->getDouble("veto_risk_score");

            result["totalScore"] = rs->getDouble("total_score");
            result["similarityScore"] = sim_score;
            result["complementarityScore"] = comp_score;
            result["vetoRiskScore"] = veto_score;

            // Derived dimensions for the frontend radar chart:
            // - hygieneConsistency: derived from similarity (hygiene is a major factor)
            // - scheduleOverlap: derived from similarity (sleep schedule is a major factor)
            // These are approximations since the algorithm doesn't produce separate sub-scores.
            result["hygieneConsistencyScore"] = sim_score * 0.9 + 0.05; // scale ~0.05-0.95
            result["scheduleOverlapScore"] = sim_score * 0.85 + 0.1;    // scale ~0.1-0.95

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
                        Escape(rid.get<std::string>()) + "';");
                    if (detail_rs->next())
                    {
                        nlohmann::json rm;
                        rm["userId"] = detail_rs->getString("user_id").asStdString();
                        rm["studentNo"] = detail_rs->getString("student_no").asStdString();
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

        // ---- GetQuestionnaireTemplate ----
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
            tmpl["vetoItemOptions"] = {"smoke_alcohol", "midnight_gaming", "loud_speaker",
                                       "opposite_sex", "bad_hygiene", "over_demand",
                                       "boundary_violation", "pets"};
            return tmpl.dump();
        }

        // ---- Register ----
        std::string StudentService::Register(
            MySqlClient &db,
            const std::string &register_json)
        {
            auto data = nlohmann::json::parse(register_json);

            std::string student_no = data.value("studentNo", "");
            std::string password = data.value("password", "");

            if (student_no.empty() || password.empty())
            {
                throw std::runtime_error("Student number and password are required.");
            }

            // Check if student_no already exists
            auto rs = db.ExecuteQuery(
                "SELECT user_id FROM `user` WHERE student_no = '" +
                Escape(student_no) + "';");
            if (rs->next())
            {
                throw std::runtime_error("Student number already registered.");
            }

            // Generate user data
            std::string user_id = GenId("u_");
            std::string gender = data.value("gender", "");
            std::string college = data.value("college", "");
            std::string major = data.value("major", "");
            std::string grade = data.value("grade", "");
            int dorm_type = data.value("dormType", 4);

            // Hash password with random salt
            std::string salt = dorm_alloc::infra::auth::CryptoUtil::GenerateSalt();
            std::string hashed_pw = dorm_alloc::infra::auth::CryptoUtil::HashPassword(password, salt);

            std::ostringstream sql;
            sql << "INSERT INTO `user` "
                << "(user_id, student_no, password, salt, gender, college, major, grade, dorm_type, role) VALUES ("
                << "'" << user_id << "', "
                << "'" << Escape(student_no) << "', "
                << "'" << hashed_pw << "', "
                << "'" << salt << "', "
                << "'" << Escape(gender) << "', "
                << "'" << Escape(college) << "', "
                << "'" << Escape(major) << "', "
                << "'" << Escape(grade) << "', "
                << dorm_type << ", "
                << "'student');";
            db.Execute(sql.str());

            nlohmann::json result;
            result["userId"] = user_id;
            result["studentNo"] = student_no;
            result["role"] = "student";
            return result.dump();
        }

    } // namespace service
} // namespace dorm_alloc
