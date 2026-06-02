#include "infrastructure/auth/CryptoUtil.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace dorm_alloc
{
    namespace infra
    {
        namespace auth
        {

            // ============================================================
            // SHA-256 implementation (pure C++14, no external dependency)
            // ============================================================

            // Right-rotate a 32-bit value
            static inline uint32_t RotR(uint32_t x, uint32_t n)
            {
                return (x >> n) | (x << (32 - n));
            }

            // SHA-256 auxiliary functions
            static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z)
            {
                return (x & y) ^ (~x & z);
            }

            static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z)
            {
                return (x & y) ^ (x & z) ^ (y & z);
            }

            static inline uint32_t Sigma0(uint32_t x)
            {
                return RotR(x, 2) ^ RotR(x, 13) ^ RotR(x, 22);
            }

            static inline uint32_t Sigma1(uint32_t x)
            {
                return RotR(x, 6) ^ RotR(x, 11) ^ RotR(x, 25);
            }

            static inline uint32_t sigma0(uint32_t x)
            {
                return RotR(x, 7) ^ RotR(x, 18) ^ (x >> 3);
            }

            static inline uint32_t sigma1(uint32_t x)
            {
                return RotR(x, 17) ^ RotR(x, 19) ^ (x >> 10);
            }

            std::string CryptoUtil::SHA256(const std::string &input)
            {
                // SHA-256 round constants
                static const uint32_t K[64] = {
                    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
                    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
                    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

                // Initial hash values
                uint32_t H[8] = {
                    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

                // Pre-processing: pad the message
                std::vector<uint8_t> msg(input.begin(), input.end());
                uint64_t bit_len = msg.size() * 8;

                // Append 0x80
                msg.push_back(0x80);

                // Pad to 56 mod 64 bytes
                while (msg.size() % 64 != 56)
                {
                    msg.push_back(0x00);
                }

                // Append original length in bits as big-endian 64-bit
                for (int i = 7; i >= 0; --i)
                {
                    msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xFF));
                }

                // Process each 64-byte block
                for (size_t block = 0; block < msg.size(); block += 64)
                {
                    uint32_t W[64];

                    // Prepare message schedule
                    for (int t = 0; t < 16; ++t)
                    {
                        W[t] = (static_cast<uint32_t>(msg[block + t * 4]) << 24) |
                               (static_cast<uint32_t>(msg[block + t * 4 + 1]) << 16) |
                               (static_cast<uint32_t>(msg[block + t * 4 + 2]) << 8) |
                               (static_cast<uint32_t>(msg[block + t * 4 + 3]));
                    }
                    for (int t = 16; t < 64; ++t)
                    {
                        W[t] = sigma1(W[t - 2]) + W[t - 7] + sigma0(W[t - 15]) + W[t - 16];
                    }

                    // Initialize working variables
                    uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
                    uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

                    // 64 rounds
                    for (int t = 0; t < 64; ++t)
                    {
                        uint32_t T1 = h + Sigma1(e) + Ch(e, f, g) + K[t] + W[t];
                        uint32_t T2 = Sigma0(a) + Maj(a, b, c);
                        h = g;
                        g = f;
                        f = e;
                        e = d + T1;
                        d = c;
                        c = b;
                        b = a;
                        a = T1 + T2;
                    }

                    // Update hash values
                    H[0] += a;
                    H[1] += b;
                    H[2] += c;
                    H[3] += d;
                    H[4] += e;
                    H[5] += f;
                    H[6] += g;
                    H[7] += h;
                }

                // Convert to hex string
                std::ostringstream oss;
                for (int i = 0; i < 8; ++i)
                {
                    oss << std::hex << std::setfill('0') << std::setw(8) << H[i];
                }
                return oss.str();
            }

            std::string CryptoUtil::GenerateSalt()
            {
                // Seed with time + address for some entropy
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();
                std::srand(static_cast<unsigned>(ms ^ reinterpret_cast<uintptr_t>(&ms)));

                const char hex[] = "0123456789abcdef";
                std::string salt;
                salt.reserve(32);
                for (int i = 0; i < 32; ++i)
                {
                    salt += hex[std::rand() % 16];
                }
                return salt;
            }

            std::string CryptoUtil::HashPassword(const std::string &password,
                                                 const std::string &salt)
            {
                return SHA256(salt + password);
            }

            bool CryptoUtil::VerifyPassword(const std::string &password,
                                            const std::string &salt,
                                            const std::string &expected_hash)
            {
                return HashPassword(password, salt) == expected_hash;
            }

        } // namespace auth
    } // namespace infra
} // namespace dorm_alloc