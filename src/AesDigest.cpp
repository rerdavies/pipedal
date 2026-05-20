#include "AesDigest.hpp"

#include <array>
#include <openssl/evp.h>
#include <stdexcept>

using namespace pipedal;


std::string pipedal::AesDigest(const std::string &text)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    size_t digestLength = 0;

    if (EVP_Q_digest(
            nullptr,
            "SHA256",
            nullptr,
            reinterpret_cast<const unsigned char *>(text.data()),
            text.size(),
            digest.data(),
            &digestLength)
        != 1)
    {
        throw std::runtime_error("EVP_Q_digest(SHA256) failed.");
    }

    static constexpr char hexDigits[] = "0123456789abcdef";
    std::string result(digestLength * 2, '\0');
    for (size_t i = 0; i < digestLength; ++i)
    {
        unsigned char value = digest[i];
        result[2 * i] = hexDigits[(value >> 4) & 0x0F];
        result[2 * i + 1] = hexDigits[value & 0x0F];
    }
    return result;
}



