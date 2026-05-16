#include "algorithm/MatchEngine.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

#include <mysql/jdbc.h>

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
                "  q.gaming_behavior, q.mbti_type, "
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

        // TODO: Implement actual similarity calculation.
        // Currently returns a placeholder score based on simple matching.
        double MatchEngine::CalculateSimilarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            // Stub: simple attribute matching score
            // TODO: Replace with actual similarity algorithm
            double score = 0.0;
            int total_fields = 0;

            // Sleep schedule match
            if (!a.sleep_schedule.empty() && !b.sleep_schedule.empty())
            {
                total_fields++;
                if (a.sleep_schedule == b.sleep_schedule)
                    score += 1.0;
            }

            // Gaming behavior match
            if (!a.gaming_behavior.empty() && !b.gaming_behavior.empty())
            {
                total_fields++;
                if (a.gaming_behavior == b.gaming_behavior)
                    score += 1.0;
            }

            // MBTI compatibility (simple match)
            if (!a.mbti_type.empty() && !b.mbti_type.empty())
            {
                total_fields++;
                if (a.mbti_type == b.mbti_type)
                    score += 1.0;
            }

            if (total_fields == 0)
                return 0.5;

            return score / total_fields;
        }

        // TODO: Implement actual complementarity calculation.
        double MatchEngine::CalculateComplementarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            // Stub: placeholder based on numeric field differences
            // TODO: Replace with actual algorithm
            (void)a;
            (void)b;
            return 0.5;
        }

        bool MatchEngine::HasVetoConflict(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            // Check if a's veto items conflict with b's profile
            // For simplicity: if a has veto items that match b's gaming_behavior
            for (const auto &item : a.veto_items)
            {
                if (item == b.gaming_behavior)
                    return true;
                if (item == b.sleep_schedule)
                    return true;
            }
            // Check reverse
            for (const auto &item : b.veto_items)
            {
                if (item == a.gaming_behavior)
                    return true;
                if (item == a.sleep_schedule)
                    return true;
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

                // Find the best pair of unassigned students
                int best_i = -1, best_j = -1;
                double best_score = -2.0;

                for (size_t i = 0; i < n; ++i)
                {
                    if (assigned[i])
                        continue;
                    for (size_t j = i + 1; j < n; ++j)
                    {
                        if (assigned[j])
                            continue;
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
                        if (assigned[k])
                            continue;

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

            // Handle remaining unassigned students - put them in remaining dorms
            for (size_t i = 0; i < n; ++i)
            {
                if (!assigned[i])
                {
                    // Find a dorm that still has space
                    std::string dorm_id = "unassigned";
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
                            break;
                        }
                    }

                    AllocationResult r;
                    r.result_id = GenerateId();
                    r.user_id = students[i].user_id;
                    r.dorm_id = dorm_id;
                    r.total_score = 0.0;
                    r.explanation_text = "Assigned as remaining student (no optimal match found).";
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

            // Update task status
            db.Execute(
                "UPDATE allocation_task SET status = 'completed', "
                "finished_at = NOW() WHERE task_id = '" +
                task_id + "';");

            return results;
        }

    } // namespace algorithm
} // namespace dorm_alloc