#include "algorithm/MatchEngine.h"
#include "infrastructure/log/Logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

#include <mysql/jdbc.h>

#include <nlohmann/json.hpp>

namespace dorm_alloc
{
    namespace algorithm
    {

        std::string MatchEngine::GenerateId()
        {
            // Simple ID: timestamp + random number
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
            int rand_part = std::rand() % 10000;
            std::ostringstream oss;
            oss << "id_" << ms << "_" << rand_part;
            return oss.str();
        }

        std::vector<StudentProfile> MatchEngine::LoadStudentProfiles(
            MySqlClient &db,
            const std::string &college,
            const std::string &major,
            const std::string &gender)
        {
            std::string sql =
                "SELECT u.user_id, u.gender, u.college, u.major, u.dorm_type, "
                "  q.sleep_schedule, q.hygiene_level, q.noise_tolerance, "
                "  q.temperature_preference, q.social_preference, "
                "  q.gaming_behavior, q.mbti_type, q.raw_answers, "
                "  p.similarity_weight, p.complementarity_weight, p.veto_safety_weight "
                "FROM `user` u "
                "INNER JOIN questionnaire q ON u.user_id = q.user_id "
                "LEFT JOIN preference p ON u.user_id = p.user_id "
                "WHERE 1=1";

            if (!college.empty())
                sql += " AND u.college = '" + college + "'";
            if (!major.empty())
                sql += " AND u.major = '" + major + "'";
            if (!gender.empty())
                sql += " AND u.gender = '" + gender + "'";
            sql += ";";

            auto rs = db.ExecuteQuery(sql);

            std::vector<StudentProfile> profiles;
            while (rs->next())
            {
                StudentProfile p;
                p.user_id = rs->getString("user_id").asStdString();
                p.gender = rs->getString("gender").asStdString();
                p.college = rs->getString("college").asStdString();
                p.major = rs->getString("major").asStdString();
                p.dorm_type = rs->getInt("dorm_type");
                p.sleep_schedule = rs->getString("sleep_schedule").asStdString();
                p.hygiene_level = rs->getInt("hygiene_level");
                p.noise_tolerance = rs->getInt("noise_tolerance");
                p.temperature_preference = rs->getInt("temperature_preference");
                p.social_preference = rs->getInt("social_preference");
                p.gaming_behavior = rs->getString("gaming_behavior").asStdString();
                p.mbti_type = rs->getString("mbti_type").asStdString();
                p.raw_answers = rs->getString("raw_answers").asStdString();

                // Preference weights may be NULL (LEFT JOIN)
                p.similarity_weight = rs->getDouble("similarity_weight");
                if (rs->wasNull())
                    p.similarity_weight = 0.5;
                p.complementarity_weight = rs->getDouble("complementarity_weight");
                if (rs->wasNull())
                    p.complementarity_weight = 0.2;
                p.veto_safety_weight = rs->getDouble("veto_safety_weight");
                if (rs->wasNull())
                    p.veto_safety_weight = 0.3;

                profiles.push_back(std::move(p));
            }

            // Load veto items for each student
            for (auto &profile : profiles)
            {
                auto veto_rs = db.ExecuteQuery(
                    "SELECT veto_item FROM veto WHERE user_id = '" + profile.user_id + "';");
                while (veto_rs->next())
                {
                    profile.veto_items.push_back(veto_rs->getString("veto_item").asStdString());
                }
            }

            return profiles;
        }

        std::vector<DormInfo> MatchEngine::LoadDormitories(
            MySqlClient &db,
            const std::string &gender)
        {
            std::string sql =
                "SELECT dorm_id, building, room_number, capacity, gender "
                "FROM dormitory WHERE is_available = 1";

            if (!gender.empty())
                sql += " AND (gender IS NULL OR gender = '' OR gender = '" + gender + "')";
            sql += ";";

            auto rs = db.ExecuteQuery(sql);

            std::vector<DormInfo> dorms;
            while (rs->next())
            {
                DormInfo d;
                d.dorm_id = rs->getString("dorm_id").asStdString();
                d.building = rs->getString("building").asStdString();
                d.room_number = rs->getString("room_number").asStdString();
                d.capacity = rs->getInt("capacity");
                d.gender = rs->getString("gender").asStdString();
                dorms.push_back(std::move(d));
            }
            return dorms;
        }

        // Map sleep schedule string to a numeric value for distance calculation.
        // early=1.0, normal=2.0, late=3.0, very_late=4.0
        double MatchEngine::MapSleepToNumeric(const std::string &schedule)
        {
            if (schedule == "early")
                return 1.0;
            if (schedule == "normal")
                return 2.0;
            if (schedule == "late")
                return 3.0;
            if (schedule == "very_late")
                return 4.0;
            return 2.5; // unknown defaults to middle
        }

        // Map gaming behavior string to a numeric value.
        // never=1.0, sometimes=2.0, often=3.0, always=4.0
        double MatchEngine::MapGamingToNumeric(const std::string &behavior)
        {
            if (behavior == "never")
                return 1.0;
            if (behavior == "sometimes")
                return 2.0;
            if (behavior == "often")
                return 3.0;
            if (behavior == "always")
                return 4.0;
            return 2.5; // unknown defaults to middle
        }

        // Calculate similarity score using normalized Euclidean distance
        // across 6 dimensions: sleep, hygiene, noise, temperature, social, gaming.
        // Returns a value in [0, 1] where 1 = identical habits.
        double MatchEngine::CalculateSimilarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            // Each dimension has a known range for normalization
            // sleep:          1-4 (mapped from string)
            // hygiene:        1-5
            // noise:          1-5
            // temperature:   18-30
            // social:         1-5
            // gaming:         1-4 (mapped from string)

            struct DimRange
            {
                double val_a;
                double val_b;
                double min_val;
                double max_val;
                bool valid;
            };

            std::vector<DimRange> dims;

            // Sleep schedule (string → numeric)
            if (!a.sleep_schedule.empty() && !b.sleep_schedule.empty())
            {
                dims.push_back({MapSleepToNumeric(a.sleep_schedule),
                                MapSleepToNumeric(b.sleep_schedule), 1.0, 4.0, true});
            }

            // Hygiene level (1-5)
            if (a.hygiene_level > 0 && b.hygiene_level > 0)
            {
                dims.push_back({(double)a.hygiene_level, (double)b.hygiene_level, 1.0, 5.0, true});
            }

            // Noise tolerance (1-5)
            if (a.noise_tolerance > 0 && b.noise_tolerance > 0)
            {
                dims.push_back({(double)a.noise_tolerance, (double)b.noise_tolerance, 1.0, 5.0, true});
            }

            // Temperature preference (18-30)
            if (a.temperature_preference > 0 && b.temperature_preference > 0)
            {
                dims.push_back({(double)a.temperature_preference, (double)b.temperature_preference, 18.0, 30.0, true});
            }

            // Social preference (1-5)
            if (a.social_preference > 0 && b.social_preference > 0)
            {
                dims.push_back({(double)a.social_preference, (double)b.social_preference, 1.0, 5.0, true});
            }

            // Gaming behavior (string → numeric 1-4)
            if (!a.gaming_behavior.empty() && !b.gaming_behavior.empty())
            {
                dims.push_back({MapGamingToNumeric(a.gaming_behavior),
                                MapGamingToNumeric(b.gaming_behavior), 1.0, 4.0, true});
            }

            if (dims.empty())
                return 0.5; // no data available

            // Calculate normalized Euclidean distance
            double sum_sq = 0.0;
            for (const auto &d : dims)
            {
                double range = d.max_val - d.min_val;
                if (range <= 0.0)
                    continue;
                double norm_diff = (d.val_a - d.val_b) / range;
                sum_sq += norm_diff * norm_diff;
            }

            double euclidean_dist = std::sqrt(sum_sq / dims.size());

            // Convert distance to similarity: similarity = 1 / (1 + distance)
            // distance=0 → similarity=1.0 (identical)
            // distance=1 → similarity=0.5 (max normalized diff per dimension)
            return 1.0 / (1.0 + euclidean_dist);
        }

        // Calculate MBTI complementarity for a single dimension.
        // Same letter → low complementarity, different letter → high complementarity.
        double MatchEngine::MbtiDimensionScore(char a, char b)
        {
            if (a == b)
                return 0.2; // same preference, low complementarity
            // Check if they are a valid MBTI pair on the same dimension
            if ((a == 'E' && b == 'I') || (a == 'I' && b == 'E') ||
                (a == 'S' && b == 'N') || (a == 'N' && b == 'S') ||
                (a == 'T' && b == 'F') || (a == 'F' && b == 'T') ||
                (a == 'J' && b == 'P') || (a == 'P' && b == 'J'))
            {
                return 1.0; // perfect complementarity on this dimension
            }
            return 0.3; // unrelated letters, slight complementarity
        }

        // Calculate complementarity score based on MBTI and social preference.
        // MBTI complementarity counts for 60%, social preference difference counts for 40%.
        // Returns a value in [0, 1].
        double MatchEngine::CalculateComplementarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            double score = 0.0;
            double total_weight = 0.0;

            // MBTI complementarity (weight = 0.6)
            if (a.mbti_type.size() >= 4 && b.mbti_type.size() >= 4)
            {
                double mbti_score = 0.0;
                for (size_t i = 0; i < 4; ++i)
                {
                    mbti_score += MbtiDimensionScore(a.mbti_type[i], b.mbti_type[i]);
                }
                mbti_score /= 4.0; // average over 4 dimensions
                score += 0.6 * mbti_score;
                total_weight += 0.6;
            }

            // Social preference complementarity (weight = 0.4)
            // A social butterfly + a lone wolf = good complementarity
            if (a.social_preference > 0 && b.social_preference > 0)
            {
                double diff = std::abs((double)a.social_preference - (double)b.social_preference);
                // Normalize: max diff is 4 (1 vs 5), map to [0, 1]
                double social_comp = diff / 4.0;
                score += 0.4 * social_comp;
                total_weight += 0.4;
            }

            if (total_weight == 0.0)
                return 0.5; // no data

            return score / total_weight;
        }

        // Derive behavior tags from a student's questionnaire data.
        // These tags represent the student's actual behaviors that could
        // conflict with another student's veto items.
        std::vector<std::string> MatchEngine::DeriveBehaviorTags(const StudentProfile &p)
        {
            std::vector<std::string> tags;

            // Parse raw_answers JSON to get explicit behavior flags
            nlohmann::json raw;
            bool has_raw = false;
            if (!p.raw_answers.empty())
            {
                try { raw = nlohmann::json::parse(p.raw_answers); has_raw = true; }
                catch (...) {}
            }

            // Navigate to traditionalHabits if this is frontend format
            nlohmann::json th;
            if (has_raw && raw.contains("traditionalHabits"))
                th = raw["traditionalHabits"];
            else if (has_raw)
                th = raw;

            // smoke_alcohol: from q10_smokeStatus (often/sometimes)
            if (has_raw)
            {
                std::string smoke = th.value("q10_smokeStatus", "");
                if (smoke == "often" || smoke == "sometimes")
                    tags.push_back("smoke_alcohol");
            }

            // midnight_gaming: games often/always AND sleeps very late
            if ((p.gaming_behavior == "often" || p.gaming_behavior == "always") &&
                (p.sleep_schedule == "very_late" || p.sleep_schedule == "late"))
            {
                tags.push_back("midnight_gaming");
            }

            // loud_speaker: high noise tolerance + high social preference
            if (p.noise_tolerance >= 4 && p.social_preference >= 4)
            {
                tags.push_back("loud_speaker");
            }

            // bad_hygiene: very low hygiene level
            if (p.hygiene_level == 1)
            {
                tags.push_back("bad_hygiene");
            }

            // over_demand: very high social + very high hygiene (demanding roommate)
            if (p.social_preference >= 5 && p.hygiene_level >= 5)
            {
                tags.push_back("over_demand");
            }

            // boundary_violation: high social + low noise tolerance
            if (p.social_preference >= 4 && p.noise_tolerance <= 2)
            {
                tags.push_back("boundary_violation");
            }

            // pets: from q11_petPreference (have/want)
            if (has_raw)
            {
                std::string pet = th.value("q11_petPreference", "");
                if (pet == "have" || pet == "want")
                    tags.push_back("pets");
            }

            // opposite_sex: from q12_oppositeSex (often/sometimes)
            if (has_raw)
            {
                std::string opp = th.value("q12_oppositeSex", "");
                if (opp == "often" || opp == "sometimes")
                    tags.push_back("opposite_sex");
            }

            return tags;
        }

        // Check veto conflicts: if student A has veto items that match
        // student B's actual behaviors (or vice versa), they conflict.
        bool MatchEngine::HasVetoConflict(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            auto tags_b = DeriveBehaviorTags(b);
            for (const auto &veto : a.veto_items)
            {
                for (const auto &tag : tags_b)
                {
                    if (veto == tag)
                        return true;
                }
            }

            auto tags_a = DeriveBehaviorTags(a);
            for (const auto &veto : b.veto_items)
            {
                for (const auto &tag : tags_a)
                {
                    if (veto == tag)
                        return true;
                }
            }

            return false;
        }

        double MatchEngine::CalculatePairScore(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            if (HasVetoConflict(a, b))
                return -1.0; // Veto conflict

            double sim = CalculateSimilarity(a, b);
            double comp = CalculateComplementarity(a, b);
            double veto_safe = 1.0; // No conflict means safe

            return a.similarity_weight * sim +
                   a.complementarity_weight * comp +
                   a.veto_safety_weight * veto_safe;
        }

        std::vector<AllocationResult> MatchEngine::GreedyAssign(
            const std::vector<StudentProfile> &students,
            const std::vector<DormInfo> &dorms)
        {
            std::vector<AllocationResult> results;

            if (students.empty() || dorms.empty())
                return results;

            // Build pair scores for all student pairs
            size_t n = students.size();
            std::map<std::pair<int, int>, double> pair_scores;

            for (size_t i = 0; i < n; ++i)
            {
                for (size_t j = i + 1; j < n; ++j)
                {
                    double score = CalculatePairScore(students[i], students[j]);
                    pair_scores[{(int)i, (int)j}] = score;
                }
            }

            // Greedy assignment: fill dorms one by one
            std::vector<bool> assigned(n, false);
            size_t dorm_idx = 0;

            for (const auto &dorm : dorms)
            {
                if (dorm_idx >= dorms.size())
                    break;

                // Find the best pair of unassigned students matching dorm gender
                int best_i = -1, best_j = -1;
                double best_score = -2.0;

                for (size_t i = 0; i < n; ++i)
                {
                    if (assigned[i]) continue;
                    // Gender filter: if dorm has gender requirement, skip non-matching
                    if (!dorm.gender.empty() && students[i].gender != dorm.gender) continue;
                    for (size_t j = i + 1; j < n; ++j)
                    {
                        if (assigned[j]) continue;
                        if (!dorm.gender.empty() && students[j].gender != dorm.gender) continue;
                        auto it = pair_scores.find({(int)i, (int)j});
                        if (it != pair_scores.end() && it->second > best_score)
                        {
                            best_score = it->second;
                            best_i = (int)i;
                            best_j = (int)j;
                        }
                    }
                }

                if (best_i < 0)
                    break; // No more pairs available

                // Start a group with the best pair
                std::vector<int> group = {best_i, best_j};
                assigned[best_i] = true;
                assigned[best_j] = true;

                // Try to fill up to dorm capacity
                int capacity = dorm.capacity;
                while ((int)group.size() < capacity)
                {
                    int best_k = -1;
                    double best_group_score = -2.0;

                    for (size_t k = 0; k < n; ++k)
                    {
                        if (assigned[k]) continue;
                        if (!dorm.gender.empty() && students[k].gender != dorm.gender) continue;

                        // Calculate average score with current group
                        double avg_score = 0.0;
                        int count = 0;
                        for (int gi : group)
                        {
                            int lo = std::min(gi, (int)k);
                            int hi = std::max(gi, (int)k);
                            auto it = pair_scores.find({lo, hi});
                            if (it != pair_scores.end())
                            {
                                avg_score += it->second;
                                count++;
                            }
                        }
                        if (count > 0)
                            avg_score /= count;

                        if (avg_score > best_group_score)
                        {
                            best_group_score = avg_score;
                            best_k = (int)k;
                        }
                    }

                    if (best_k < 0 || best_group_score < 0)
                        break; // No suitable student found

                    group.push_back(best_k);
                    assigned[best_k] = true;
                }

                // Create result entries for this group
                std::vector<std::string> member_ids;
                for (int gi : group)
                {
                    member_ids.push_back(students[gi].user_id);
                }

                for (int gi : group)
                {
                    AllocationResult r;
                    r.result_id = GenerateId();
                    r.user_id = students[gi].user_id;
                    r.dorm_id = dorm.dorm_id;
                    r.similarity_score = 0.0;
                    r.complementarity_score = 0.0;
                    r.veto_risk_score = 0.0;

                    // Calculate average scores
                    double total = 0.0;
                    int cnt = 0;
                    for (int gj : group)
                    {
                        if (gj == gi)
                            continue;
                        r.roommate_ids.push_back(students[gj].user_id);
                        int lo = std::min(gi, gj);
                        int hi = std::max(gi, gj);
                        auto it = pair_scores.find({lo, hi});
                        if (it != pair_scores.end())
                        {
                            total += it->second;
                            cnt++;
                            // Use average pair scores for breakdown
                            r.similarity_score += CalculateSimilarity(students[gi], students[gj]);
                            r.complementarity_score += CalculateComplementarity(students[gi], students[gj]);
                            r.veto_risk_score += HasVetoConflict(students[gi], students[gj]) ? 0.0 : 1.0;
                        }
                    }

                    if (cnt > 0)
                    {
                        r.total_score = total / cnt;
                        r.similarity_score /= cnt;
                        r.complementarity_score /= cnt;
                        r.veto_risk_score /= cnt;
                    }

                    // Generate explanation
                    std::ostringstream expl;
                    expl << "Assigned to " << dorm.building << " " << dorm.room_number
                         << " with " << r.roommate_ids.size() << " roommate(s). "
                         << "Match score: " << std::fixed << std::setprecision(2) << r.total_score;
                    r.explanation_text = expl.str();

                    results.push_back(std::move(r));
                }

                dorm_idx++;
            }

            // Handle remaining unassigned students - put them in dorms with space
            for (size_t i = 0; i < n; ++i)
            {
                if (!assigned[i])
                {
                    // Find a dorm that still has space
                    std::string dorm_id;
                    std::string dorm_building;
                    std::string dorm_room;
                    for (const auto &dorm : dorms)
                    {
                        // Check if this dorm already has less than capacity members
                        int count = 0;
                        for (const auto &r : results)
                        {
                            if (r.dorm_id == dorm.dorm_id)
                                count++;
                        }
                        if (count < dorm.capacity)
                        {
                            dorm_id = dorm.dorm_id;
                            dorm_building = dorm.building;
                            dorm_room = dorm.room_number;
                            break;
                        }
                    }

                    // If all dorms are full, use the last dorm (overflow)
                    if (dorm_id.empty() && !dorms.empty())
                    {
                        dorm_id = dorms.back().dorm_id;
                        dorm_building = dorms.back().building;
                        dorm_room = dorms.back().room_number;
                    }

                    AllocationResult r;
                    r.result_id = GenerateId();
                    r.user_id = students[i].user_id;
                    r.dorm_id = dorm_id;
                    r.total_score = 0.0;

                    // Generate explanation with actual dorm info
                    std::ostringstream expl;
                    if (!dorm_building.empty())
                    {
                        expl << "Assigned to " << dorm_building << " " << dorm_room
                             << " as remaining student (no optimal match found).";
                    }
                    else
                    {
                        expl << "Assigned as remaining student (no dorm available).";
                    }
                    r.explanation_text = expl.str();
                    results.push_back(std::move(r));
                }
            }

            return results;
        }

        std::vector<AllocationResult> MatchEngine::ExecuteAllocation(
            MySqlClient &db,
            const std::string &task_id,
            const std::string &college,
            const std::string &major,
            const std::string &gender,
            const std::string & /*rule_config*/)
        {
            // Load data
            auto students = LoadStudentProfiles(db, college, major, gender);
            auto dorms = LoadDormitories(db, gender);

            LOG_INFO("MatchEngine::ExecuteAllocation: {} students, {} dorms, college={}, major={}, gender={}",
                     students.size(), dorms.size(), college, major, gender);

            if (students.empty())
            {
                throw std::runtime_error("No students found for the given criteria.");
            }
            if (dorms.empty())
            {
                throw std::runtime_error("No available dormitories found.");
            }

            // Run greedy assignment
            auto results = GreedyAssign(students, dorms);
            LOG_INFO("MatchEngine::ExecuteAllocation: {} results generated", results.size());

            // Save results to database
            for (const auto &r : results)
            {
                // Build roommate_ids string
                std::string roommate_str;
                for (size_t i = 0; i < r.roommate_ids.size(); ++i)
                {
                    if (i > 0)
                        roommate_str += ",";
                    roommate_str += r.roommate_ids[i];
                }

                std::ostringstream sql;
                sql << std::fixed << std::setprecision(4);
                sql << "INSERT INTO match_result "
                    << "(result_id, task_id, user_id, dorm_id, roommate_ids, "
                    << " total_score, similarity_score, complementarity_score, veto_risk_score, "
                    << " explanation_text) VALUES ("
                    << "'" << r.result_id << "', "
                    << "'" << task_id << "', "
                    << "'" << r.user_id << "', "
                    << "'" << r.dorm_id << "', "
                    << "'" << roommate_str << "', "
                    << r.total_score << ", "
                    << r.similarity_score << ", "
                    << r.complementarity_score << ", "
                    << r.veto_risk_score << ", "
                    << "'" << r.explanation_text << "');";

                db.Execute(sql.str());
            }

            LOG_INFO("MatchEngine::ExecuteAllocation: saving {} results to database", results.size());

            // Update task status
            db.Execute(
                "UPDATE allocation_task SET status = 'completed', "
                "finished_at = NOW() WHERE task_id = '" +
                task_id + "';");

            return results;
        }

    } // namespace algorithm
} // namespace dorm_alloc