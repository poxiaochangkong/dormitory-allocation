#pragma once

// Student-facing business logic.
// Handles login, questionnaire submission, and result queries.

#include <string>

#include "infrastructure/db/MySqlClient.h"

namespace dorm_alloc
{
    namespace service
    {

        using MySqlClient = dorm_alloc::infra::db::MySqlClient;

        class StudentService
        {
        public:
            // Login with student number and password.
            // Returns JSON: {userId, studentNo, role, gender, college, major, grade, dormType, token}
            // Throws on failure.
            static std::string Login(MySqlClient &db,
                                     const std::string &student_no,
                                     const std::string &password);

            // Get student basic info by userId.
            // Returns JSON: {userId, studentNo, gender, college, major, grade, dormType}
            static std::string GetStudentInfo(MySqlClient &db,
                                              const std::string &user_id);

            // Check whether the student has submitted the questionnaire.
            // Returns JSON: {submitted: bool, questionnaireId: string|null}
            static std::string GetQuestionnaireStatus(MySqlClient &db,
                                                      const std::string &user_id);

            // Submit questionnaire data for a student.
            // Accepts frontend format: {basicInfo, traditionalHabits, vetoSettings, personality}
            // Also accepts backend format: {basicInfo, questionnaire, preference, vetoItems, openText}
            // Returns JSON: {questionnaireId: "..."}
            static std::string SubmitQuestionnaire(MySqlClient &db,
                                                   const std::string &user_id,
                                                   const std::string &data_json);

            // Submit immersive scene data for a student.
            // Returns JSON: {status: "ok"}
            static std::string SubmitSceneData(MySqlClient &db,
                                               const std::string &user_id,
                                               const std::string &data_json);

            // Get match result for a student.
            // Returns JSON with room info, roommates, scores (5 dimensions), and explanation.
            static std::string GetMatchResult(MySqlClient &db,
                                              const std::string &user_id);

            // Get questionnaire template (static placeholder).
            static std::string GetQuestionnaireTemplate();

        private:
            // Generate a simple random token string.
            static std::string GenerateToken();
        };

    } // namespace service
} // namespace dorm_alloc
