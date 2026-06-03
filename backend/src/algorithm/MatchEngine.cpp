#include "algorithm/MatchEngine.h"
#include "infrastructure/log/Logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <set>
#include <random>
#include <utility>

#include <mysql/jdbc.h>

#include <nlohmann/json.hpp>

namespace dorm_alloc
{
    namespace algorithm
    {

        // SQL string escape helper
        static std::string Esc(const std::string &s)
        {
            std::string r;
            for (char c : s)
            {
                if (c == '\'')
                    r += "''";
                else
                    r += c;
            }
            return r;
        }

        std::string MatchEngine::GenerateId()
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
            int rand_part = std::rand() % 10000;
            std::ostringstream oss;
            oss << "id_" << ms << "_" << rand_part;
            return oss.str();
        }

        // ================================================================
        // Data loading (unchanged)
        // ================================================================

        std::vector<StudentProfile> MatchEngine::LoadStudentProfiles(
            MySqlClient &db,
            const std::string &college,
            const std::string &major,
            const std::string &gender,
            const std::string &rule_config)
        {
            double fallback_sim = 0.5, fallback_comp = 0.2, fallback_veto = 0.3;
            if (!rule_config.empty())
            {
                try
                {
                    auto rc = nlohmann::json::parse(rule_config);
                    fallback_sim = rc.value("similarityWeight", 0.5);
                    fallback_comp = rc.value("complementarityWeight", 0.2);
                    fallback_veto = rc.value("vetoSafetyWeight", 0.3);
                }
                catch (...)
                {
                }
            }
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

                p.similarity_weight = rs->getDouble("similarity_weight");
                if (rs->wasNull())
                    p.similarity_weight = fallback_sim;
                p.complementarity_weight = rs->getDouble("complementarity_weight");
                if (rs->wasNull())
                    p.complementarity_weight = fallback_comp;
                p.veto_safety_weight = rs->getDouble("veto_safety_weight");
                if (rs->wasNull())
                    p.veto_safety_weight = fallback_veto;

                profiles.push_back(std::move(p));
            }

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

        // ================================================================
        // Mapping helpers (unchanged)
        // ================================================================

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
            return 2.5;
        }

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
            return 2.5;
        }

        // ================================================================
        // Scoring: Gaussian-kernel similarity (v2.0)
        // ================================================================

        double MatchEngine::CalculateSimilarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            struct DimRange
            {
                double val_a;
                double val_b;
                double min_val;
                double max_val;
            };

            std::vector<DimRange> dims;

            if (!a.sleep_schedule.empty() && !b.sleep_schedule.empty())
                dims.push_back({MapSleepToNumeric(a.sleep_schedule),
                                MapSleepToNumeric(b.sleep_schedule), 1.0, 4.0});

            if (a.hygiene_level > 0 && b.hygiene_level > 0)
                dims.push_back({(double)a.hygiene_level, (double)b.hygiene_level, 1.0, 5.0});

            if (a.noise_tolerance > 0 && b.noise_tolerance > 0)
                dims.push_back({(double)a.noise_tolerance, (double)b.noise_tolerance, 1.0, 5.0});

            if (a.temperature_preference > 0 && b.temperature_preference > 0)
                dims.push_back({(double)a.temperature_preference, (double)b.temperature_preference, 18.0, 30.0});

            if (a.social_preference > 0 && b.social_preference > 0)
                dims.push_back({(double)a.social_preference, (double)b.social_preference, 1.0, 5.0});

            if (!a.gaming_behavior.empty() && !b.gaming_behavior.empty())
                dims.push_back({MapGamingToNumeric(a.gaming_behavior),
                                MapGamingToNumeric(b.gaming_behavior), 1.0, 4.0});

            if (dims.empty())
                return 0.5;

            // Normalized Euclidean distance squared, averaged over dimensions
            double sum_sq = 0.0;
            for (const auto &d : dims)
            {
                double range = d.max_val - d.min_val;
                if (range <= 0.0)
                    continue;
                double norm_diff = (d.val_a - d.val_b) / range;
                sum_sq += norm_diff * norm_diff;
            }

            double avg_sq_dist = sum_sq / dims.size();

            // Gaussian kernel: exp(-d^2 / (2 * sigma^2)), sigma = 0.3
            const double sigma = 0.3;
            const double two_sigma_sq = 2.0 * sigma * sigma; // 0.18
            return std::exp(-avg_sq_dist / two_sigma_sq);
        }

        // ================================================================
        // Scoring: MBTI cognitive function stack complementarity (v2.0)
        // ================================================================

        std::vector<std::string> MatchEngine::GetCognitiveFunctions(const std::string &mbti_type)
        {
            // 16 MBTI types → top-4 cognitive functions
            static const std::map<std::string, std::vector<std::string>> func_map = {
                {"ISTJ", {"Si", "Te", "Fi", "Ne"}},
                {"ISFJ", {"Si", "Fe", "Ti", "Ne"}},
                {"INFJ", {"Ni", "Fe", "Ti", "Se"}},
                {"INTJ", {"Ni", "Te", "Fi", "Se"}},
                {"ISTP", {"Ti", "Se", "Ni", "Fe"}},
                {"ISFP", {"Fi", "Se", "Ni", "Te"}},
                {"INFP", {"Fi", "Ne", "Si", "Te"}},
                {"INTP", {"Ti", "Ne", "Si", "Fe"}},
                {"ESTP", {"Se", "Ti", "Fe", "Ni"}},
                {"ESFP", {"Se", "Fi", "Te", "Ni"}},
                {"ENFP", {"Ne", "Fi", "Te", "Si"}},
                {"ENTP", {"Ne", "Ti", "Fe", "Si"}},
                {"ESTJ", {"Te", "Si", "Ne", "Fi"}},
                {"ESFJ", {"Fe", "Si", "Ne", "Ti"}},
                {"ENFJ", {"Fe", "Ni", "Se", "Ti"}},
                {"ENTJ", {"Te", "Ni", "Se", "Fi"}}};

            auto it = func_map.find(mbti_type);
            if (it != func_map.end())
                return it->second;

            // Unknown type: return empty
            return {};
        }

        double MatchEngine::FunctionComplementarity(const std::string &func_a, const std::string &func_b)
        {
            // Same function → conflict (0.0)
            if (func_a == func_b)
                return 0.0;

            // Perceiving function complement: Ni↔Ne, Si↔Se
            if ((func_a == "Ni" && func_b == "Ne") || (func_a == "Ne" && func_b == "Ni") ||
                (func_a == "Si" && func_b == "Se") || (func_a == "Se" && func_b == "Si"))
                return 0.5;

            // Judging function complement: Ti↔Te, Fi↔Fe
            if ((func_a == "Ti" && func_b == "Te") || (func_a == "Te" && func_b == "Ti") ||
                (func_a == "Fi" && func_b == "Fe") || (func_a == "Fe" && func_b == "Fi"))
                return 0.5;

            // Different domain (e.g., Ni-Ti, Se-Fe) → partial complement
            return 0.2;
        }

        double MatchEngine::CalculateComplementarity(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            auto funcs_a = GetCognitiveFunctions(a.mbti_type);
            auto funcs_b = GetCognitiveFunctions(b.mbti_type);

            if (funcs_a.empty() || funcs_b.empty())
                return 0.5; // no MBTI data

            // Sum complementarity over all 4×4 = 16 function pairs
            double total = 0.0;
            for (const auto &fa : funcs_a)
            {
                for (const auto &fb : funcs_b)
                {
                    total += FunctionComplementarity(fa, fb);
                }
            }

            // Normalize to [0, 1]: max possible = 16 × 0.5 = 8.0
            return total / 8.0;
        }

        // ================================================================
        // Veto / behavior tags (unchanged logic)
        // ================================================================

        std::vector<std::string> MatchEngine::DeriveBehaviorTags(const StudentProfile &p)
        {
            std::vector<std::string> tags;

            nlohmann::json raw;
            bool has_raw = false;
            if (!p.raw_answers.empty())
            {
                try
                {
                    raw = nlohmann::json::parse(p.raw_answers);
                    has_raw = true;
                }
                catch (...)
                {
                }
            }

            nlohmann::json th;
            if (has_raw && raw.contains("traditionalHabits"))
                th = raw["traditionalHabits"];
            else if (has_raw)
                th = raw;

            if (has_raw)
            {
                std::string smoke = th.value("q10_smokeStatus", "");
                if (smoke == "often" || smoke == "sometimes")
                    tags.push_back("smoke_alcohol");
            }

            if ((p.gaming_behavior == "often" || p.gaming_behavior == "always") &&
                (p.sleep_schedule == "very_late" || p.sleep_schedule == "late"))
            {
                tags.push_back("midnight_gaming");
            }

            if (p.noise_tolerance >= 4 && p.social_preference >= 4)
                tags.push_back("loud_speaker");

            if (p.hygiene_level == 1)
                tags.push_back("bad_hygiene");

            if (p.social_preference >= 5 && p.hygiene_level >= 5)
                tags.push_back("over_demand");

            if (p.social_preference >= 4 && p.noise_tolerance <= 2)
                tags.push_back("boundary_violation");

            if (has_raw)
            {
                std::string pet = th.value("q11_petPreference", "");
                if (pet == "have" || pet == "want")
                    tags.push_back("pets");
            }

            if (has_raw)
            {
                std::string opp = th.value("q12_oppositeSex", "");
                if (opp == "often" || opp == "sometimes")
                    tags.push_back("opposite_sex");
            }

            return tags;
        }

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

        // ================================================================
        // Pair score with symmetrized weights (v2.0)
        // ================================================================

        double MatchEngine::CalculatePairScore(
            const StudentProfile &a,
            const StudentProfile &b)
        {
            if (HasVetoConflict(a, b))
                return -1.0;

            double sim = CalculateSimilarity(a, b);
            double comp = CalculateComplementarity(a, b);
            double veto_safe = 1.0;

            // Symmetrize: use average of both students' weights
            double w_sim = (a.similarity_weight + b.similarity_weight) / 2.0;
            double w_comp = (a.complementarity_weight + b.complementarity_weight) / 2.0;
            double w_veto = (a.veto_safety_weight + b.veto_safety_weight) / 2.0;

            return w_sim * sim + w_comp * comp + w_veto * veto_safe;
        }

        // ================================================================
        // Room / global scoring
        // ================================================================

        double MatchEngine::ComputeRoomScore(
            const std::vector<int> &room,
            const std::map<std::pair<int, int>, double> &pair_scores)
        {
            if (room.size() < 2)
                return 0.0;

            std::vector<double> scores;
            for (size_t i = 0; i < room.size(); ++i)
            {
                for (size_t j = i + 1; j < room.size(); ++j)
                {
                    int lo = std::min(room[i], room[j]);
                    int hi = std::max(room[i], room[j]);
                    auto it = pair_scores.find({lo, hi});
                    scores.push_back(it != pair_scores.end() ? it->second : 0.0);
                }
            }

            double avg = 0.0;
            for (double s : scores)
                avg += s;
            avg /= scores.size();

            double mn = *std::min_element(scores.begin(), scores.end());

            return 0.7 * avg + 0.3 * mn;
        }

        double MatchEngine::ComputeGlobalScore(
            const std::vector<std::vector<int>> &rooms,
            const std::map<std::pair<int, int>, double> &pair_scores)
        {
            double total = 0.0;
            for (const auto &room : rooms)
            {
                if (room.size() >= 2)
                    total += ComputeRoomScore(room, pair_scores);
            }
            return total;
        }

        // ================================================================
        // Phase 1: Pair Formation — greedy approximate max-weight matching
        // ================================================================

        std::vector<std::pair<int, int>> MatchEngine::FormPairs(
            const std::vector<int> &indices,
            const std::map<std::pair<int, int>, double> &pair_scores)
        {
            if (indices.size() < 2)
                return {};

            // Collect all valid candidate pairs (score > 0)
            struct Candidate
            {
                double score;
                int i, j;
            };
            std::vector<Candidate> candidates;

            for (size_t a = 0; a < indices.size(); ++a)
            {
                for (size_t b = a + 1; b < indices.size(); ++b)
                {
                    int lo = std::min(indices[a], indices[b]);
                    int hi = std::max(indices[a], indices[b]);
                    auto it = pair_scores.find({lo, hi});
                    if (it != pair_scores.end() && it->second > 0)
                    {
                        candidates.push_back({it->second, indices[a], indices[b]});
                    }
                }
            }

            // Sort descending by score
            std::sort(candidates.begin(), candidates.end(),
                      [](const Candidate &x, const Candidate &y)
                      { return x.score > y.score; });

            // Greedy matching
            std::set<int> assigned;
            std::vector<std::pair<int, int>> pairs;

            for (const auto &c : candidates)
            {
                if (assigned.count(c.i) == 0 && assigned.count(c.j) == 0)
                {
                    pairs.push_back({c.i, c.j});
                    assigned.insert(c.i);
                    assigned.insert(c.j);
                }
            }

            return pairs;
        }

        // ================================================================
        // Phase 2: Pair Merging — combine pairs into rooms
        // ================================================================

        std::vector<std::vector<int>> MatchEngine::MergePairsIntoRooms(
            const std::vector<std::pair<int, int>> &pairs,
            const std::vector<int> &unpaired,
            const std::vector<StudentProfile> &students,
            const std::map<std::pair<int, int>, double> &pair_scores,
            int room_capacity)
        {
            std::vector<std::vector<int>> rooms;

            // Cross-compatibility between two pairs: average of 4 cross pair_scores
            auto cross_compat = [&](const std::pair<int, int> &pA,
                                    const std::pair<int, int> &pB) -> double
            {
                int a1 = pA.first, a2 = pA.second;
                int b1 = pB.first, b2 = pB.second;

                auto get_score = [&](int x, int y) -> double
                {
                    int lo = std::min(x, y), hi = std::max(x, y);
                    auto it = pair_scores.find({lo, hi});
                    return it != pair_scores.end() ? it->second : 0.0;
                };

                double s1 = get_score(a1, b1);
                double s2 = get_score(a1, b2);
                double s3 = get_score(a2, b1);
                double s4 = get_score(a2, b2);

                // Any veto conflict → reject
                if (s1 < 0 || s2 < 0 || s3 < 0 || s4 < 0)
                    return -1.0;

                return (s1 + s2 + s3 + s4) / 4.0;
            };

            // Collect merge candidates
            struct MergeCand
            {
                double compat;
                int i, j;
            };
            std::vector<MergeCand> merge_cands;

            for (size_t i = 0; i < pairs.size(); ++i)
            {
                for (size_t j = i + 1; j < pairs.size(); ++j)
                {
                    double compat = cross_compat(pairs[i], pairs[j]);
                    if (compat > 0)
                    {
                        merge_cands.push_back({compat, (int)i, (int)j});
                    }
                }
            }

            std::sort(merge_cands.begin(), merge_cands.end(),
                      [](const MergeCand &a, const MergeCand &b)
                      { return a.compat > b.compat; });

            std::set<int> merged;
            for (const auto &mc : merge_cands)
            {
                if (merged.count(mc.i) == 0 && merged.count(mc.j) == 0)
                {
                    rooms.push_back({pairs[mc.i].first, pairs[mc.i].second,
                                     pairs[mc.j].first, pairs[mc.j].second});
                    merged.insert(mc.i);
                    merged.insert(mc.j);
                }
            }

            // Remaining un-merged pairs → students go to leftover pool
            std::vector<int> leftover(unpaired.begin(), unpaired.end());
            for (size_t i = 0; i < pairs.size(); ++i)
            {
                if (merged.count((int)i) == 0)
                {
                    leftover.push_back(pairs[i].first);
                    leftover.push_back(pairs[i].second);
                }
            }

            // Place leftover students: try to fill existing rooms, then create new ones
            for (int s : leftover)
            {
                bool placed = false;
                // First: try to add to an existing room with space
                for (auto &room : rooms)
                {
                    if ((int)room.size() < room_capacity)
                    {
                        // Check gender + no veto conflict
                        bool ok = true;
                        for (int m : room)
                        {
                            if (students[m].gender != students[s].gender)
                            {
                                ok = false;
                                break;
                            }
                            int lo = std::min(m, s), hi = std::max(m, s);
                            auto it = pair_scores.find({lo, hi});
                            if (it != pair_scores.end() && it->second < 0)
                            {
                                ok = false;
                                break;
                            }
                        }
                        if (ok)
                        {
                            room.push_back(s);
                            placed = true;
                            break;
                        }
                    }
                }
                if (!placed)
                {
                    rooms.push_back({s});
                }
            }

            return rooms;
        }

        // ================================================================
        // Phase 3: Simulated Annealing Refinement
        // ================================================================

        std::vector<std::vector<int>> MatchEngine::RefineBySA(
            std::vector<std::vector<int>> rooms,
            const std::vector<StudentProfile> &students,
            const std::map<std::pair<int, int>, double> &pair_scores,
            int room_capacity)
        {
            // Handle trivial cases
            if (rooms.size() <= 1)
                return rooms;

            // SA parameters
            const double T_init = 1.0;
            const double T_min = 0.001;
            const double alpha = 0.997;
            int n = 0;
            for (const auto &r : rooms)
                n += (int)r.size();
            int iter_per_T = std::max(n, 10);

            std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
            std::uniform_real_distribution<double> uniform(0.0, 1.0);

            double current_score = ComputeGlobalScore(rooms, pair_scores);
            auto best_rooms = rooms;
            double best_score = current_score;

            double T = T_init;
            while (T > T_min)
            {
                for (int iter = 0; iter < iter_per_T; ++iter)
                {
                    // Pick two distinct rooms
                    if (rooms.size() < 2)
                        break;
                    int r1_idx = rng() % rooms.size();
                    int r2_idx = rng() % rooms.size();
                    if (r1_idx == r2_idx)
                        continue;
                    if (rooms[r1_idx].empty() || rooms[r2_idx].empty())
                        continue;

                    // Pick one student from each room
                    int s1_pos = rng() % rooms[r1_idx].size();
                    int s2_pos = rng() % rooms[r2_idx].size();
                    int s1 = rooms[r1_idx][s1_pos];
                    int s2 = rooms[r2_idx][s2_pos];

                    // Validate swap: gender constraint + veto constraint
                    bool valid = true;

                    // Check s2 fits into room r1
                    for (int m : rooms[r1_idx])
                    {
                        if (m == s1)
                            continue;
                        if (students[m].gender != students[s2].gender)
                        {
                            valid = false;
                            break;
                        }
                        int lo = std::min(m, s2), hi = std::max(m, s2);
                        auto it = pair_scores.find({lo, hi});
                        if (it != pair_scores.end() && it->second < 0)
                        {
                            valid = false;
                            break;
                        }
                    }
                    if (!valid)
                        continue;

                    // Check s1 fits into room r2
                    for (int m : rooms[r2_idx])
                    {
                        if (m == s2)
                            continue;
                        if (students[m].gender != students[s1].gender)
                        {
                            valid = false;
                            break;
                        }
                        int lo = std::min(m, s1), hi = std::max(m, s1);
                        auto it = pair_scores.find({lo, hi});
                        if (it != pair_scores.end() && it->second < 0)
                        {
                            valid = false;
                            break;
                        }
                    }
                    if (!valid)
                        continue;

                    // Compute scores before swap (only affected rooms)
                    double old_r1 = ComputeRoomScore(rooms[r1_idx], pair_scores);
                    double old_r2 = ComputeRoomScore(rooms[r2_idx], pair_scores);

                    // Execute swap
                    rooms[r1_idx][s1_pos] = s2;
                    rooms[r2_idx][s2_pos] = s1;

                    // Compute scores after swap
                    double new_r1 = ComputeRoomScore(rooms[r1_idx], pair_scores);
                    double new_r2 = ComputeRoomScore(rooms[r2_idx], pair_scores);

                    double delta = (new_r1 + new_r2) - (old_r1 + old_r2);

                    // Metropolis criterion
                    if (delta > 0 || uniform(rng) < std::exp(delta / T))
                    {
                        current_score += delta;
                        if (current_score > best_score)
                        {
                            best_score = current_score;
                            best_rooms = rooms;
                        }
                    }
                    else
                    {
                        // Rollback swap
                        rooms[r1_idx][s1_pos] = s1;
                        rooms[r2_idx][s2_pos] = s2;
                    }
                }

                T *= alpha;
            }

            LOG_INFO("RefineBySA: initial_score={:.4f}, best_score={:.4f}", current_score, best_score);
            return best_rooms;
        }

        // ================================================================
        // Main entry point: ExecuteAllocation (v2.0)
        // ================================================================

        std::vector<AllocationResult> MatchEngine::ExecuteAllocation(
            MySqlClient &db,
            const std::string &task_id,
            const std::string &college,
            const std::string &major,
            const std::string &gender,
            const std::string &rule_config)
        {
            // Load data
            auto students = LoadStudentProfiles(db, college, major, gender, rule_config);
            auto dorms = LoadDormitories(db, gender);

            LOG_INFO("MatchEngine::ExecuteAllocation: {} students, {} dorms, college={}, major={}, gender={}",
                     students.size(), dorms.size(), college, major, gender);

            if (students.empty())
                throw std::runtime_error("No students found for the given criteria.");
            if (dorms.empty())
                throw std::runtime_error("No available dormitories found.");

            // ===== Audit Step 1: Data stats =====
            {
                nlohmann::json stats;
                stats["studentCount"] = (int)students.size();
                stats["dormCount"] = (int)dorms.size();
                int male = 0, female = 0;
                std::map<std::string, int> mbti_dist, veto_dist;
                double sum_hygiene = 0;
                for (const auto &s : students)
                {
                    if (s.gender == "male")
                        male++;
                    else
                        female++;
                    if (!s.mbti_type.empty())
                        mbti_dist[s.mbti_type]++;
                    sum_hygiene += s.hygiene_level;
                    for (const auto &v : s.veto_items)
                        veto_dist[v]++;
                }
                stats["genderDistribution"] = {{"male", male}, {"female", female}};
                stats["avgHygiene"] = (students.empty() ? 0 : sum_hygiene / students.size());
                nlohmann::json mbti_j, veto_j;
                for (const auto &kv : mbti_dist)
                    mbti_j[kv.first] = kv.second;
                for (const auto &kv : veto_dist)
                    veto_j[kv.first] = kv.second;
                stats["mbtiDistribution"] = mbti_j;
                stats["vetoTotal"] = 0;
                for (const auto &s : students)
                    stats["vetoTotal"] = stats["vetoTotal"].get<int>() + (int)s.veto_items.size();
                stats["vetoItemDistribution"] = veto_j;

                std::ostringstream asql;
                asql << "INSERT INTO algorithm_audit (audit_id, task_id, step, step_order, data_json) VALUES ('"
                     << GenerateId() << "','" << task_id << "','stats',1,'" << Esc(stats.dump()) << "');";
                db.Execute(asql.str());
            }

            // ===== Precompute all pair scores =====
            size_t n = students.size();
            std::map<std::pair<int, int>, double> pair_scores;
            for (size_t i = 0; i < n; ++i)
            {
                for (size_t j = i + 1; j < n; ++j)
                {
                    pair_scores[{(int)i, (int)j}] = CalculatePairScore(students[i], students[j]);
                }
            }

            // ===== Phase 1: Pair Formation =====
            // Group students by gender, form pairs within each group
            std::map<std::string, std::vector<int>> gender_groups;
            for (size_t i = 0; i < n; ++i)
                gender_groups[students[i].gender].push_back((int)i);

            std::vector<std::pair<int, int>> all_pairs;
            std::vector<int> all_unpaired;

            for (auto it = gender_groups.begin(); it != gender_groups.end(); ++it)
            {
                const std::vector<int> &indices = it->second;
                auto pairs = FormPairs(indices, pair_scores);
                std::set<int> paired_set;
                for (auto &p : pairs)
                {
                    all_pairs.push_back(p);
                    paired_set.insert(p.first);
                    paired_set.insert(p.second);
                }
                for (int idx : indices)
                {
                    if (paired_set.count(idx) == 0)
                        all_unpaired.push_back(idx);
                }
            }

            LOG_INFO("Phase 1 (Pair Formation): {} pairs, {} unpaired", all_pairs.size(), all_unpaired.size());

            // ===== Phase 2: Pair Merging =====
            int room_cap = 4;
            if (!dorms.empty())
                room_cap = dorms[0].capacity;
            if (room_cap <= 0)
                room_cap = 4;

            auto rooms = MergePairsIntoRooms(all_pairs, all_unpaired, students, pair_scores, room_cap);

            LOG_INFO("Phase 2 (Pair Merging): {} rooms formed", rooms.size());

            // ===== Phase 3: SA Refinement =====
            auto final_rooms = RefineBySA(rooms, students, pair_scores, room_cap);

            double sa_score = ComputeGlobalScore(final_rooms, pair_scores);
            LOG_INFO("Phase 3 (SA Refinement): global_score={:.4f}", sa_score);

            // ===== Map rooms to dormitories =====
            std::vector<AllocationResult> results;

            // Sort rooms by size descending for better dorm assignment
            std::vector<size_t> room_order(final_rooms.size());
            std::iota(room_order.begin(), room_order.end(), 0);
            std::sort(room_order.begin(), room_order.end(),
                      [&](size_t a, size_t b)
                      { return final_rooms[a].size() > final_rooms[b].size(); });

            std::vector<int> dorm_remaining(dorms.size());
            for (size_t d = 0; d < dorms.size(); ++d)
                dorm_remaining[d] = dorms[d].capacity;

            for (size_t ri : room_order)
            {
                const auto &room = final_rooms[ri];
                if (room.empty())
                    continue;

                // Find a suitable dorm
                int best_dorm = -1;
                for (size_t d = 0; d < dorms.size(); ++d)
                {
                    if (dorm_remaining[d] >= (int)room.size())
                    {
                        // Gender check
                        if (!dorms[d].gender.empty() &&
                            students[room[0]].gender != dorms[d].gender)
                            continue;
                        best_dorm = (int)d;
                        break;
                    }
                }

                // Fallback: if no dorm has enough space for the full room,
                // find a partially-filled dorm with same gender that can take some students
                if (best_dorm < 0)
                {
                    for (size_t d = 0; d < dorms.size(); ++d)
                    {
                        if (dorm_remaining[d] > 0)
                        {
                            if (!dorms[d].gender.empty() &&
                                students[room[0]].gender != dorms[d].gender)
                                continue;
                            best_dorm = (int)d;
                            break;
                        }
                    }
                }

                std::string dorm_id, dorm_building, dorm_room;
                if (best_dorm >= 0)
                {
                    dorm_id = dorms[best_dorm].dorm_id;
                    dorm_building = dorms[best_dorm].building;
                    dorm_room = dorms[best_dorm].room_number;
                    dorm_remaining[best_dorm] -= (int)room.size();
                }
                else
                {
                    // No dorm available — log warning, skip these students from results
                    LOG_WARN("No dorm available for room with {} students (dorms full or gender mismatch), skipping from match_result", room.size());
                    for (int s : room)
                    {
                        AllocationResult r;
                        r.result_id = GenerateId();
                        r.user_id = students[s].user_id;
                        r.dorm_id = "";
                        r.total_score = 0;
                        r.explanation_text = "Unassigned: no available dormitory.";
                        results.push_back(std::move(r));
                    }
                    continue; // Skip DB insert for this room
                }

                // Generate results for each student in the room
                for (int s : room)
                {
                    AllocationResult r;
                    r.result_id = GenerateId();
                    r.user_id = students[s].user_id;
                    r.dorm_id = dorm_id;

                    double total = 0.0;
                    int cnt = 0;
                    for (int m : room)
                    {
                        if (m == s)
                            continue;
                        r.roommate_ids.push_back(students[m].user_id);
                        int lo = std::min(s, m), hi = std::max(s, m);
                        auto it = pair_scores.find({lo, hi});
                        if (it != pair_scores.end())
                        {
                            total += it->second;
                            cnt++;
                        }
                        r.similarity_score += CalculateSimilarity(students[s], students[m]);
                        r.complementarity_score += CalculateComplementarity(students[s], students[m]);
                        r.veto_risk_score += HasVetoConflict(students[s], students[m]) ? 0.0 : 1.0;
                    }

                    if (cnt > 0)
                    {
                        r.total_score = total / cnt;
                        r.similarity_score /= cnt;
                        r.complementarity_score /= cnt;
                        r.veto_risk_score /= cnt;
                    }

                    std::ostringstream expl;
                    expl << "Assigned to " << dorm_building << " " << dorm_room
                         << " with " << r.roommate_ids.size() << " roommate(s). "
                         << "Match score: " << std::fixed << std::setprecision(2) << r.total_score;
                    r.explanation_text = expl.str();

                    results.push_back(std::move(r));
                }
            }

            LOG_INFO("MatchEngine::ExecuteAllocation: {} results generated", results.size());

            // Save results to database (skip unassigned students — dorm_id is NOT NULL)
            int saved_count = 0;
            for (const auto &r : results)
            {
                if (r.dorm_id.empty())
                {
                    LOG_WARN("Skipping DB insert for unassigned student: {}", r.user_id);
                    continue;
                }

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
                    << "'" << Esc(r.explanation_text) << "');";

                db.Execute(sql.str());
                saved_count++;
            }

            LOG_INFO("MatchEngine::ExecuteAllocation: saved {}/{} results to database ({} unassigned)",
                     saved_count, results.size(), (int)results.size() - saved_count);

            // ===== Audit Step 2: Veto conflict summary =====
            {
                nlohmann::json veto_audit;
                std::map<std::string, int> conflict_count;
                int total_conflicts = 0;
                for (size_t i = 0; i < n; ++i)
                {
                    auto tags_a = DeriveBehaviorTags(students[i]);
                    for (size_t j = i + 1; j < n; ++j)
                    {
                        auto tags_b = DeriveBehaviorTags(students[j]);
                        bool conflict = false;
                        for (const auto &v : students[i].veto_items)
                            for (const auto &t : tags_b)
                                if (v == t)
                                {
                                    conflict = true;
                                    conflict_count[v]++;
                                }
                        for (const auto &v : students[j].veto_items)
                            for (const auto &t : tags_a)
                                if (v == t)
                                {
                                    conflict = true;
                                    conflict_count[v]++;
                                }
                        if (conflict)
                            total_conflicts++;
                    }
                }
                veto_audit["totalPairs"] = (int)(n * (n - 1) / 2);
                veto_audit["vetoConflicts"] = total_conflicts;
                veto_audit["conflictRate"] = (n < 2 ? 0.0 : (double)total_conflicts / (n * (n - 1) / 2));
                nlohmann::json top_v;
                std::vector<std::pair<int, std::string>> sorted_v;
                for (const auto &kv : conflict_count)
                    sorted_v.push_back({kv.second, kv.first});
                std::sort(sorted_v.rbegin(), sorted_v.rend());
                for (const auto &p : sorted_v)
                    top_v.push_back({{"item", p.second}, {"conflicts", p.first}});
                veto_audit["topConflictItems"] = top_v;

                std::ostringstream asql;
                asql << "INSERT INTO algorithm_audit (audit_id, task_id, step, step_order, data_json) VALUES ('"
                     << GenerateId() << "','" << task_id << "','veto_matrix',2,'" << Esc(veto_audit.dump()) << "');";
                db.Execute(asql.str());
            }

            // ===== Audit Step 3: Group formation summary =====
            {
                nlohmann::json groups_audit;
                std::map<std::string, std::vector<double>> dorm_scores_map;
                for (const auto &r : results)
                {
                    dorm_scores_map[r.dorm_id].push_back(r.total_score);
                }
                nlohmann::json gs = nlohmann::json::array();
                double sum_all = 0;
                int count_all = 0;
                for (const auto &d : dorms)
                {
                    auto it = dorm_scores_map.find(d.dorm_id);
                    if (it != dorm_scores_map.end() && !it->second.empty())
                    {
                        double avg = 0;
                        for (double s : it->second)
                            avg += s;
                        avg /= it->second.size();
                        gs.push_back({{"dorm", d.building + "-" + d.room_number},
                                      {"students", (int)it->second.size()},
                                      {"avgScore", avg}});
                        sum_all += avg * it->second.size();
                        count_all += (int)it->second.size();
                    }
                }
                groups_audit["groupsFormed"] = (int)gs.size();
                groups_audit["overflowStudents"] = 0;
                for (const auto &r : results)
                    if (r.total_score == 0.0)
                        groups_audit["overflowStudents"] = groups_audit["overflowStudents"].get<int>() + 1;
                groups_audit["avgGroupScore"] = (count_all > 0 ? sum_all / count_all : 0.0);
                groups_audit["groupScores"] = gs;
                groups_audit["saGlobalScore"] = sa_score;

                std::ostringstream asql;
                asql << "INSERT INTO algorithm_audit (audit_id, task_id, step, step_order, data_json) VALUES ('"
                     << GenerateId() << "','" << task_id << "','groups',3,'" << Esc(groups_audit.dump()) << "');";
                db.Execute(asql.str());
            }

            // ===== Audit Step 4: Final summary =====
            {
                nlohmann::json final_audit;
                final_audit["totalAllocated"] = (int)results.size();
                double ts = 0, ss = 0, cs = 0, vs = 0;
                for (const auto &r : results)
                {
                    ts += r.total_score;
                    ss += r.similarity_score;
                    cs += r.complementarity_score;
                    vs += r.veto_risk_score;
                }
                int sz = std::max(1, (int)results.size());
                final_audit["avgTotalScore"] = ts / sz;
                final_audit["avgSimilarityScore"] = ss / sz;
                final_audit["avgComplementarityScore"] = cs / sz;
                final_audit["avgVetoRiskScore"] = vs / sz;

                std::ostringstream asql;
                asql << "INSERT INTO algorithm_audit (audit_id, task_id, step, step_order, data_json) VALUES ('"
                     << GenerateId() << "','" << task_id << "','final',4,'" << Esc(final_audit.dump()) << "');";
                db.Execute(asql.str());
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