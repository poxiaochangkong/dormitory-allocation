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
            // Returns a JSON string: {"userId":"...","role":"student","gender":"..."}
            // Throws on failure.
            static std::string Login(MySqlClient &db,
                                     const std::string &student_no,
                                     const std::string &password);

            // Submit questionnaire data for a student.
            // The data JSON contains basicInfo, questionnaire, and preference sections.
            // Returns a JSON string: {"questionnaireId":"..."}
            static std::string SubmitQuestionnaire(MySqlClient &db,
                                                   const std::string &user_id,
                                                   const std::string &data_json);

            // Get match result for a student.
            // Returns a JSON string with the result or empty object if no result.
            static std::string GetMatchResult(MySqlClient &db,
                                              const std::string &user_id);

            // Get questionnaire template (static placeholder).
            static std::string GetQuestionnaireTemplate();
        };

    } // namespace service
} // namespace dorm_alloc