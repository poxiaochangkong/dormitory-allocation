#pragma once

// Matching engine for dormitory allocation.
//
// This module handles:
// - Veto filtering
// - Similarity/complementarity scoring (TODO: actual algorithm)
// - Dormitory assignment
//
// The actual similarity calculation is left as a stub for future implementation.

#include <string>
#include <vector>

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
            // Load student profiles with questionnaire data for the given scope.
            static std::vector<StudentProfile> LoadStudentProfiles(
                MySqlClient &db,
                const std::string &college,
                const std::string &major,
                const std::string &gender);

            // Load available dormitories for the given scope.
            static std::vector<DormInfo> LoadDormitories(
                MySqlClient &db,
                const std::string &gender);

            // TODO: Calculate similarity score between two students.
            // This is the core algorithm to be implemented.
            static double CalculateSimilarity(
                const StudentProfile &a,
                const StudentProfile &b);

            // TODO: Calculate complementarity score between two students.
            static double CalculateComplementarity(
                const StudentProfile &a,
                const StudentProfile &b);

            // Check veto conflicts between two students.
            static bool HasVetoConflict(
                const StudentProfile &a,
                const StudentProfile &b);

            // Calculate overall pair score.
            static double CalculatePairScore(
                const StudentProfile &a,
                const StudentProfile &b);

            // Greedy assignment algorithm.
            static std::vector<AllocationResult> GreedyAssign(
                const std::vector<StudentProfile> &students,
                const std::vector<DormInfo> &dorms);

            // Generate a UUID-like string for IDs.
            static std::string GenerateId();
        };

    } // namespace algorithm
} // namespace dorm_alloc