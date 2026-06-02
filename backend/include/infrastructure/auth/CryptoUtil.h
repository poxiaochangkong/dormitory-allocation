#pragma once

// Simple cryptographic utilities for password hashing.
// Uses SHA-256 with per-user random salt.

#include <string>

namespace dorm_alloc
{
    namespace infra
    {
        namespace auth
        {

            class CryptoUtil
            {
            public:
                // Generate a random hex salt string (32 hex chars = 16 bytes).
                static std::string GenerateSalt();

                // Hash a password with the given salt using SHA-256.
                // Returns 64-char hex string: SHA256(salt + password).
                static std::string HashPassword(const std::string &password,
                                                const std::string &salt);

                // Verify a password against stored hash and salt.
                static bool VerifyPassword(const std::string &password,
                                           const std::string &salt,
                                           const std::string &expected_hash);

                // Compute SHA-256 of the input string, returned as 64-char lowercase hex.
                static std::string SHA256(const std::string &input);
            };

        } // namespace auth
    } // namespace infra
} // namespace dorm_alloc