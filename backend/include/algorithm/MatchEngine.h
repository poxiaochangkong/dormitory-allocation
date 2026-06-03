#pragma once

// Matching engine for dormitory allocation.
//
// Three-phase algorithm (v2.0):
//   Phase 1: Pair Formation  — greedy approximate max-weight matching
//   Phase 2: Pair Merging    — merge pairs into 4-person rooms
//   Phase 3: SA Refinement   — simulated annealing global optimization
//
// Scoring:
//   - Gaussian-kernel similarity (6 dimensions)
//   - MBTI cognitive-function-stack complementarity
//   - Symmetrized preference weights

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "infrastructure/db/MySqlClient.h"

namespace dorm_alloc
{
    namespace algorithm
    {

        // Alias for convenience
        using MySqlClient = dorm_alloc::infra::db::MySqlClient;

        struct StudentProfile
        {
            std::string user_id;
            std::string gender;
            std::string college;
            std::string major;
            int dorm_type = 4;
            // Questionnaire fields
            std::string sleep_schedule;
            int hygiene_level = 0;
            int noise_tolerance = 0;
            int temperature_preference = 0;
            int social_preference = 0;
            std::string gaming_behavior;
            std::string mbti_type;
            // Preference weights
            double similarity_weight = 0.5;
            double complementarity_weight = 0.2;
            double veto_safety_weight = 0.3;
            // Raw questionnaire answers (JSON) for veto derivation
            std::string raw_answers;
            // Veto items
            std::vector<std::string> veto_items;
        };

        struct DormInfo
        {
            std::string dorm_id;
            std::string building;
            std::string room_number;
            int capacity = 4;
            std::string gender;
        };

        struct AllocationResult
        {
            std::string result_id;
            std::string user_id;
            std::string dorm_id;
            std::vector<std::string> roommate_ids;
            double total_score = 0.0;
            double similarity_score = 0.0;
            double complementarity_score = 0.0;
            double veto_risk_score = 0.0;
            std::string explanation_text;
        };

        class MatchEngine
        {
        public:
            // Execute the full allocation algorithm for a given task.
            // Returns allocation results for all students in the task scope.
            static std::vector<AllocationResult> ExecuteAllocation(
                MySqlClient &db,
                const std::string &task_id,
                const std::string &college,
                const std::string &major,
                const std::string &gender,
                const std::string &rule_config);

        private:
            // --- Data loading ---
            static std::vector<StudentProfile> LoadStudentProfiles(
                MySqlClient &db,
                const std::string &college,
                const std::string &major,
                const std::string &gender,
                const std::string &rule_config = "");

            static std::vector<DormInfo> LoadDormitories(
                MySqlClient &db,
                const std::string &gender);

            // --- Scoring functions ---
            static double MapSleepToNumeric(const std::string &schedule);
            static double MapGamingToNumeric(const std::string &behavior);

            // Gaussian-kernel similarity across 6 normalized dimensions.
            static double CalculateSimilarity(
                const StudentProfile &a,
                const StudentProfile &b);

            // MBTI cognitive-function-stack complementarity.
            // Returns the top-4 cognitive functions for a given MBTI type.
            static std::vector<std::string> GetCognitiveFunctions(const std::string &mbti_type);

            // Scores complementarity between two individual cognitive functions.
            static double FunctionComplementarity(const std::string &func_a, const std::string &func_b);

            // Overall complementarity score based on cognitive function stacks.
            static double CalculateComplementarity(
                const StudentProfile &a,
                const StudentProfile &b);

            // Behavior tag derivation and veto conflict detection.
            static std::vector<std::string> DeriveBehaviorTags(const StudentProfile &p);
            static bool HasVetoConflict(
                const StudentProfile &a,
                const StudentProfile &b);

            // Overall pair score with symmetrized weights.
            static double CalculatePairScore(
                const StudentProfile &a,
                const StudentProfile &b);

            // --- Three-phase allocation ---

            // Phase 1: Greedy approximate max-weight matching → pairs + unpaired.
            static std::vector<std::pair<int, int>> FormPairs(
                const std::vector<int> &indices,
                const std::map<std::pair<int, int>, double> &pair_scores);

            // Phase 2: Merge pairs into rooms of capacity, handle leftovers.
            static std::vector<std::vector<int>> MergePairsIntoRooms(
                const std::vector<std::pair<int, int>> &pairs,
                const std::vector<int> &unpaired,
                const std::vector<StudentProfile> &students,
                const std::map<std::pair<int, int>, double> &pair_scores,
                int room_capacity);

            // Phase 3: Simulated annealing refinement.
            static std::vector<std::vector<int>> RefineBySA(
                std::vector<std::vector<int>> rooms,
                const std::vector<StudentProfile> &students,
                const std::map<std::pair<int, int>, double> &pair_scores,
                int room_capacity);

            // --- Room / global scoring ---
            static double ComputeRoomScore(
                const std::vector<int> &room,
                const std::map<std::pair<int, int>, double> &pair_scores);

            static double ComputeGlobalScore(
                const std::vector<std::vector<int>> &rooms,
                const std::map<std::pair<int, int>, double> &pair_scores);

            // --- Utility ---
            static std::string GenerateId();
        };

    } // namespace algorithm
} // namespace dorm_alloc